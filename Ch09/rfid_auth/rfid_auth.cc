#include <my_global.h>
#include <mysql/plugin_auth.h>
#include <mysql/client_plugin.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <mysql.h>

#ifndef __WIN__

#include <unistd.h>
#include <pwd.h>

#else

#include <windows.h>
#include <conio.h>
#include <iostream>
#include <string>
using namespace std;

/*
 Get password with no echo.
*/
char *getpass(const char *prompt)
{
  string pass ="";
  char ch;
  cout << prompt;
  ch = _getch();
  while(ch != 13) //character 13 is enter key 
  {
    pass.push_back(ch);
    ch = _getch();
  }
  return (char *)pass.c_str();
}

#endif

#define MAX_RFID_CODE  12
#define MAX_BUFFER    255
#define MAX_PIN        16

/* Client-side plugin  */


/*
 * Read a RFID code from the serial port.
 */
#ifndef __WIN__
unsigned char *get_rfid_code(char *port)
{
  int fd;
  unsigned char *rfid_code= NULL;
  int nbytes;
  unsigned char raw_buff[MAX_BUFFER];
  unsigned char *bufptr = NULL;

  fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
  {
    printf("Unable to open port: %s.\n", port);
    return NULL;
  }
  else
    fcntl(fd, F_SETFL, 0);
    
  bufptr = raw_buff;
  while ((nbytes = read(fd, bufptr, raw_buff + sizeof(raw_buff) - bufptr - 1)) > 0)
  {
	bufptr += nbytes;
    if (bufptr[-1] == '\n' || bufptr[-2] == '\n' || bufptr[-3] == '\n' || 
        bufptr[-1] == '\r' || bufptr[-2] == '\r' || bufptr[-3] == '\r' ||
        bufptr[-1] == 0x03 || bufptr[-2] == 0x03 || bufptr[-3] == 0x03)
      break;
  }
  *bufptr = '\0';

  rfid_code = (unsigned char *)strdup((char *)raw_buff);
  return rfid_code;
}

#else

unsigned char *get_rfid_code(char *port)
{
	HANDLE com_port;
  DWORD nbytes;
  unsigned char raw_buff[MAX_BUFFER];
  unsigned char *rfid_code= NULL;
	
	/* Open the port specified. */
	com_port = CreateFile(port, GENERIC_READ, 0, 0, OPEN_EXISTING,
												FILE_ATTRIBUTE_NORMAL, 0);
	if (com_port == INVALID_HANDLE_VALUE)
	{
		int error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND)
		{
	    printf("Unable to open port: %s.\n", port);
      return NULL;
		}
    printf("Error opening port: %s:%d.\n", port, error);
    return NULL;
	}
	
	/* Configure the port. */
	DCB com_config = {0};
	com_config.DCBlength = sizeof(com_config);
	if (!GetCommState(com_port, &com_config))
	{
    printf("Unable to get port state.\n");
    return NULL;
	}
	com_config.BaudRate = CBR_9600;
	com_config.ByteSize = 8;
	com_config.Parity = NOPARITY;
	com_config.StopBits = ONESTOPBIT;
	if (!SetCommState(com_port, &com_config))
	{
    printf("Unable to set port state.\n");
    return NULL;
	}
	
	/* Set timeouts. */
	COMMTIMEOUTS timeouts = {0};
	timeouts.ReadIntervalTimeout=50;
	timeouts.ReadTotalTimeoutConstant=50;
	timeouts.ReadTotalTimeoutMultiplier=10;
	if (!SetCommTimeouts(com_port, &timeouts))
	{
		printf("Cannot set timeouts for port.\n");
		return NULL;
	}
	
	/* Read from the port. */
	if (!ReadFile(com_port, raw_buff, MAX_BUFFER, &nbytes, NULL))
	{
    printf("Unable to read from the port.\n");
    return NULL;
	}
	
	/* Close the port. */
	CloseHandle(com_port);
	
  rfid_code = (unsigned char *)strdup((char *)raw_buff);
	return rfid_code;
}

#endif /* __WIN__ */

static int rfid_send(MYSQL_PLUGIN_VIO *vio, st_mysql *mysql)
{
  char *port= 0;
  char pass[MAX_PIN];
  int len, res;
  unsigned char buffer[MAX_BUFFER];
  unsigned char *raw_buff= NULL;
  int start= 0;

  /* Get the port to open. */
  port= getenv("MYSQL_RFID_PORT");
  if (!port)
  {
    printf("Environment variable not set.\n");
    return CR_ERROR;
  }

  printf("Please swipe your card now...\n");
  
  raw_buff = get_rfid_code(port);
  if (raw_buff == NULL)
  {
    printf("Cannot read RFID code.\n");
    return CR_ERROR;
  }  
  len = strlen((char *)raw_buff);

  // Strip off leading extra bytes.
  for (int j= 0; j < 2; j++)  
     if (raw_buff[j] == 0x02 || raw_buff[j] == 0x03)
       start++;
  
  strncpy((char *)buffer, (char *)raw_buff+start, len-start);
  len = strlen((char *)buffer);
  /* Check for valid read. */
  if (len >= MAX_RFID_CODE)
  {
    // Strip off extra bytes at end (CR, LF, etc)
    buffer[MAX_RFID_CODE] = '\0';
  	len = MAX_RFID_CODE;
  }
  else
  {
    printf("RFID code length error. Please try again.\n");
    return CR_ERROR;
  }
	
  strncpy(pass, getpass("Please enter your PIN: "), sizeof(pass));
  strcat((char *)buffer, pass);
  len = strlen((char *)buffer);
  
  res= vio->write_packet(vio, buffer, len);

  return res ? CR_ERROR : CR_OK;
}

mysql_declare_client_plugin(AUTHENTICATION)
  "rfid_auth",
  "Chuck Bell",
  "RFID Authentication Plugin - Client",
  {0, 0, 1},
  "GPL",
  NULL,
  NULL,
  NULL,
  NULL,
  rfid_send
mysql_end_client_plugin;


/*
 * Server-side plugin
 */
static int rfid_auth_validate(MYSQL_PLUGIN_VIO *vio,
                              MYSQL_SERVER_AUTH_INFO *info)
{
  unsigned char *pkt;
  int pkt_len, err= CR_OK;

  if ((pkt_len= vio->read_packet(vio, &pkt)) < 0)
    return CR_ERROR;

  info->password_used= PASSWORD_USED_YES;

  if (strcmp((const char *) pkt, info->auth_string))
    return CR_ERROR;

  return err;
}

static struct st_mysql_auth rfid_auth_handler=
{
  MYSQL_AUTHENTICATION_INTERFACE_VERSION,
  "rfid_auth",
  rfid_auth_validate
};

mysql_declare_plugin(rfid_auth_plugin)
{
  MYSQL_AUTHENTICATION_PLUGIN,
  &rfid_auth_handler,
  "rfid_auth",
  "Chuck Bell",
  "RFID Authentication Plugin - Server",
  PLUGIN_LICENSE_GPL,
  NULL,
  NULL,
  0x0100,
  NULL,
  NULL,
  NULL,
  0,
}
mysql_declare_plugin_end;
