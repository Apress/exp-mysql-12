#pragma unmanaged

#include "DBEngine.h"
#include <stdlib.h>
#include <stdio.h>
#include "my_global.h"
#include "mysql.h" 

MYSQL *mysql;                         //the embedded server class
MYSQL_RES *results;                   //stores results from queries
MYSQL_ROW record;                     //a single row in a result set
bool IteratorStarted;                 //used to control iterator
MYSQL_RES *ExecQuery(char *Query);

/*
  These variables set the location of the ini file and data stores.
*/
static char *server_options[] = {"mysql_test", 
  "--defaults-file=c:\\mysql_embedded\\my.ini", 
  "--datadir=c:\\mysql_embedded\\data" };
int num_elements=sizeof(server_options) / sizeof(char *);
static char *server_groups[] = {"libmyswld_server", "libmysqld_client" };

DBEngine::DBEngine(void)
{
  mysqlError = false;
}

DBEngine::~DBEngine(void)
{
}

const char *DBEngine::GetError()
{
  return (mysql_error(mysql));
  mysqlError = false;
}

bool DBEngine::Error()
{
  return(mysqlError);
}

char *DBEngine::GetBookFieldStr(int Slot, char *Field)
{
  char *istr = new char[10];
  char *str = new char[128];

  _itoa_s(Slot, istr, 10, 10);
  strcpy_s(str, 128, "SELECT ");
  strcat_s(str, 128, Field); 
  strcat_s(str, 128, " FROM books WHERE Slot = ");
  strcat_s(str, 128, istr);
  mysqlError = false;
  results=ExecQuery(str);
  strcpy_s(str, 128, "");
  if (results)
  { 
    mysqlError = false;
    record=mysql_fetch_row(results);
    if(record)
    {
      strcpy_s(str, 128, record[0]);
    }
    else
    {
      mysqlError = true;
    }
  }
  return (str);
}

char *DBEngine::GetBookFieldText(int Slot, char *Field)
{
  char *istr = new char[10];
  char *str = new char[128];

  _itoa_s(Slot, istr, 10, 10);
  strcpy_s(str, 128, "SELECT ");
  strcat_s(str, 128, Field); 
  strcat_s(str, 128, " FROM books WHERE Slot = ");
  strcat_s(str, 128, istr);
  mysqlError = false;
  results=ExecQuery(str);
  delete str;
  if (results)
  { 
    mysqlError = false;
    record=mysql_fetch_row(results);
    if(record)
    {
      return (record[0]);
    }
    else
    {
      mysqlError = true;
    }
  }
  return ("");
}

int DBEngine::GetBookFieldInt(int Slot, char *Field)
{
  char *istr = new char[10];
  char *str = new char[128];
  int qty = 0;

  _itoa_s(Slot, istr, 10, 10);
  strcpy_s(str, 128, "SELECT ");
  strcat_s(str, 128, Field); 
  strcat_s(str, 128, " FROM books WHERE Slot = ");
  strcat_s(str, 128, istr);
  results=ExecQuery(str);
  if (results)
  { 
    record=mysql_fetch_row(results);
    if(record)
    {
      qty = atoi(record[0]);
    }
    else
    {
      mysqlError = true;
    }
  }
  delete str;
  return (qty);
}

void DBEngine::VendBook(char *ISBN)
{
  char *str = new char[128];
  char *istr = new char[10];
  int qty = 0;

  strcpy_s(str, 128, "SELECT Quantity FROM books WHERE ISBN = '");
  strcat_s(str, 128, ISBN);
  strcat_s(str, 128, "'");
  results=ExecQuery(str);
  record=mysql_fetch_row(results);
  if (record)
  {
    qty = atoi(record[0]);
    if (qty >= 1)
    {
      _itoa_s(qty - 1, istr, 10, 10);
      strcpy_s(str, 128, "UPDATE books SET Quantity = ");
      strcat_s(str, 128, istr);
      strcat_s(str, 128, " WHERE ISBN = '");
      strcat_s(str, 128, ISBN);
      strcat_s(str, 128, "'");
      results=ExecQuery(str);
    }
  }
  else
  {
    mysqlError = true;
  }  
}

void DBEngine::Initialize()
{
  /*
    This section initializes the server and sets server options.
  */
  mysql_server_init(num_elements, server_options, server_groups);
  mysql = mysql_init(NULL);
  if (mysql)
  {
    mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "libmysqld_client");
    mysql_options(mysql, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);
    /*
      The following call turns debugging on programmatically.
      Comment out to turn on debugging.
    */
    //mysql_debug("d:t:i:O,\\mysqld_embedded.trace");
    /*
      Connect to embedded server.
    */
    if(mysql_real_connect(mysql, NULL, NULL, NULL, "mysql", 0, NULL, 0) == NULL)
    {
      mysqlError = true;
    }
    else
    {
      mysql_query(mysql, "use BVM;");
    }
  }
  else
  {
    mysqlError = true;
  }
  IteratorStarted = false;
}

void DBEngine::Shutdown()
{
  /*
    Now close the server connection and tell server we're done (shutdown).
  */
  mysql_close(mysql); 
  mysql_server_end();
}

char *DBEngine::GetSetting(char *Field)
{
  char *str = new char[128];
  strcpy_s(str, 128, "SELECT * FROM settings WHERE FieldName = '");
  strcat_s(str, 128, Field);
  strcat_s(str, 128, "'");
  results=ExecQuery(str);
  strcpy_s(str, 128, "");
  if (results)
  { 
    record=mysql_fetch_row(results);
    if (record)
    {
      strcpy_s(str, 128, record[1]);
    }
  }
  else
  {
    mysqlError = true;
  }
  return (str);
}

void DBEngine::StartQuery(char *QueryStatement)
{
  if (!IteratorStarted)
  {
    results=ExecQuery(QueryStatement);
    if (results)
    {
      record=mysql_fetch_row(results);
    }
  }
  IteratorStarted=true;
}

void DBEngine::RunQuery(char *QueryStatement)
{
  results=ExecQuery(QueryStatement);
  if (results)
  {
    record=mysql_fetch_row(results);
    if(!record)
    {
      mysqlError = true;
    }
  }
}

int DBEngine::GetNext()
{
  //if EOF then no more records
  IteratorStarted=false;
  record=mysql_fetch_row(results);
  if (record)
  {
    return (1);
  }
  else
  {
    return (0);
  }
}

char *DBEngine::GetField(int fldNum)
{
  if (record)
  {
    return (record[fldNum]);
  }
  else
  {
    return ("");
  }
}

MYSQL_RES *ExecQuery(char *Query)
{
  mysql_dbug_print("ExecQuery.");          
  mysql_free_result(results);
  mysql_query(mysql, Query);               
  return (mysql_store_result(mysql));     
}
#pragma managed