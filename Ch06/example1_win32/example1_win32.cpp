#include "my_global.h"
#include "mysql.h" 
 
MYSQL *mysql;                         //the embedded server class
MYSQL_RES *results;                   //stores results from queries
MYSQL_ROW record;                     //a single row in a result set

/*
  These variables set the location of the ini file and data stores.
*/
static char *server_options[] = {"mysql_test", 
  "--defaults-file=c:\\mysql_embedded\\my.ini", 
  "--datadir=c:\\mysql_embedded\\data" };
int num_elements=sizeof(server_options) / sizeof(char *);
static char *server_groups[] = {"libmysqld_server", "libmysqld_client" };

int main(void)
{
  /*
    This section initializes the server and sets server options.
  */
  mysql_server_init(num_elements, server_options, server_groups);
  mysql = mysql_init(NULL);
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
  mysql_real_connect(mysql, NULL, NULL, NULL, "mysql", 0, NULL, 0);
  /*
    This sections executes the following commands and demonstrates
    how to retrieve results from a query.

    SHOW DATABASES;
    CREATE DATABASE testdb1;
    SHOW DATABASES;
    DROP DATABASE testdb1;
  */
  mysql_dbug_print("Showing databases.");                 //record trace
  mysql_query(mysql, "SHOW DATABASES;");                  //issue query
  results = mysql_store_result(mysql);                    //get results
  printf("The following are the databases supported:\n"); 
  while(record=mysql_fetch_row(results))                  //fetch row
  {
    printf("%s\n", record[0]);                            //process row
  }
  mysql_dbug_print("Creating the database testdb1.");     //record trace
  mysql_query(mysql, "CREATE DATABASE testdb1;");
  mysql_dbug_print("Showing databases.");
  mysql_query(mysql, "SHOW DATABASES;");                  //issue query
  results = mysql_store_result(mysql);                    //get results
  printf("The following are the databases supported:\n"); 
  while(record=mysql_fetch_row(results))                  //fetch row
  {
    printf("%s\n", record[0]);                            //process row
  }
  mysql_free_result(results);
  mysql_dbug_print("Dropping database testdb1.");         //record trace
  mysql_query(mysql, "DROP DATABASE testdb1;");           //issue query
  /*
    Now close the server connection and tell server we're done (shutdown).
  */
  mysql_close(mysql); 
  mysql_server_end();

  return 0;
}

