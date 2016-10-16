/*
  Spartan Storage Engine Plugin
*/

#include "sql_priv.h"
#include "sql_class.h"                         
#include "ha_spartan.h"
#include "probes_mysql.h"
#include "sql_plugin.h"
#include "my_sys.h"

static handler *spartan_create_handler(handlerton *hton,
                                       TABLE_SHARE *table, 
                                       MEM_ROOT *mem_root);

handlerton *spartan_hton;

/* Interface to mysqld, to check system tables supported by SE */
static const char* spartan_system_database();
static bool spartan_is_supported_system_table(const char *db,
                                      const char *table_name,
                                      bool is_sql_layer_system_table);
#ifdef HAVE_PSI_INTERFACE
static PSI_mutex_key ex_key_mutex_Spartan_share_mutex;

static PSI_mutex_info all_spartan_mutexes[]=
{
  { &ex_key_mutex_Spartan_share_mutex, "Spartan_share::mutex", 0}
};

static void init_spartan_psi_keys()
{
  const char* category= "spartan";
  int count;

  count= array_elements(all_spartan_mutexes);
  mysql_mutex_register(category, all_spartan_mutexes, count);
}
#endif

Spartan_share::Spartan_share()
{
  thr_lock_init(&lock);
  mysql_mutex_init(ex_key_mutex_Spartan_share_mutex,
                   &mutex, MY_MUTEX_INIT_FAST);
  data_class = new Spartan_data();
}


static int spartan_init_func(void *p)
{
  DBUG_ENTER("spartan_init_func");

#ifdef HAVE_PSI_INTERFACE
  init_spartan_psi_keys();
#endif

  spartan_hton= (handlerton *)p;
  spartan_hton->state=                     SHOW_OPTION_YES;
  spartan_hton->create=                    spartan_create_handler;
  spartan_hton->flags=                     HTON_CAN_RECREATE;
  spartan_hton->system_database=   spartan_system_database;
  spartan_hton->is_supported_system_table= spartan_is_supported_system_table;

  DBUG_RETURN(0);
}


/**
  @brief
  Spartan of simple lock controls. The "share" it creates is a
  structure we will pass to each spartan handler. Do you have to have
  one of these? Well, you have pieces that are used for locking, and
  they are needed to function.
*/

Spartan_share *ha_spartan::get_share()
{
  Spartan_share *tmp_share;

  DBUG_ENTER("ha_spartan::get_share()");

  lock_shared_ha_data();
  if (!(tmp_share= static_cast<Spartan_share*>(get_ha_share_ptr())))
  {
    tmp_share= new Spartan_share;
    if (!tmp_share)
      goto err;

    set_ha_share_ptr(static_cast<Handler_share*>(tmp_share));
  }
err:
  unlock_shared_ha_data();
  DBUG_RETURN(tmp_share);
}


static handler* spartan_create_handler(handlerton *hton,
                                       TABLE_SHARE *table, 
                                       MEM_ROOT *mem_root)
{
  return new (mem_root) ha_spartan(hton, table);
}

ha_spartan::ha_spartan(handlerton *hton, TABLE_SHARE *table_arg)
  :handler(hton, table_arg)
{}


/**
  @brief
  If frm_error() is called then we will use this to determine
  the file extensions that exist for the storage engine. This is also
  used by the default rename_table and delete_table method in
  handler.cc.

  For engines that have two file name extentions (separate meta/index file
  and data file), the order of elements is relevant. First element of engine
  file name extentions array should be meta/index file extention. Second
  element - data file extention. This order is assumed by
  prepare_for_repair() when REPAIR TABLE ... USE_FRM is issued.

  @see
  rename_table method in handler.cc and
  delete_table method in handler.cc
*/

#define SDE_EXT ".sde"
#define SDI_EXT ".sdi"

static const char *ha_spartan_exts[] = {
  SDE_EXT,
  SDI_EXT,
  NullS
};

const char **ha_spartan::bas_ext() const
{
  return ha_spartan_exts;
}

/*
  Following handler function provides access to
  system database specific to SE. This interface
  is optional, so every SE need not implement it.
*/
const char* ha_spartan_system_database= NULL;
const char* spartan_system_database()
{
  return ha_spartan_system_database;
}

/*
  List of all system tables specific to the SE.
  Array element would look like below,
     { "<database_name>", "<system table name>" },
  The last element MUST be,
     { (const char*)NULL, (const char*)NULL }

  This array is optional, so every SE need not implement it.
*/
static st_system_tablename ha_spartan_system_tables[]= {
  {(const char*)NULL, (const char*)NULL}
};

/**
  @brief Check if the given db.tablename is a system table for this SE.

  @param db                         Database name to check.
  @param table_name                 table name to check.
  @param is_sql_layer_system_table  if the supplied db.table_name is a SQL
                                    layer system table.

  @return
    @retval TRUE   Given db.table_name is supported system table.
    @retval FALSE  Given db.table_name is not a supported system table.
*/
static bool spartan_is_supported_system_table(const char *db,
                                              const char *table_name,
                                              bool is_sql_layer_system_table)
{
  st_system_tablename *systab;

  // Does this SE support "ALL" SQL layer system tables ?
  if (is_sql_layer_system_table)
    return false;

  // Check if this is SE layer system tables
  systab= ha_spartan_system_tables;
  while (systab && systab->db)
  {
    if (systab->db == db &&
        strcmp(systab->tablename, table_name) == 0)
      return true;
    systab++;
  }

  return false;
}


/**
  @brief
  Used for opening tables. The name will be the name of the file.

  @details
  A table is opened when it needs to be opened; e.g. when a request comes in
  for a SELECT on the table (tables are not open and closed for each request,
  they are cached).

  Called from handler.cc by handler::ha_open(). The server opens all tables by
  calling ha_open() which then calls the handler specific open().

  @see
  handler::ha_open() in handler.cc
*/

int ha_spartan::open(const char *name, int mode, uint test_if_locked)
{
  DBUG_ENTER("ha_spartan::open");
  char name_buff[FN_REFLEN];

  if (!(share = get_share()))
    DBUG_RETURN(1);
  /*
    Call the data class open table method.
    Note: the fn_format() method correctly creates a file name from the
    name passed into the method.
  */
  share->data_class->open_table(fn_format(name_buff, name, "", SDE_EXT,
                                MY_REPLACE_EXT|MY_UNPACK_FILENAME));
  thr_lock_data_init(&share->lock,&lock,NULL);
  DBUG_RETURN(0);
}


/**
  @brief
  Closes a table.

  @details
  Called from sql_base.cc, sql_select.cc, and table.cc. In sql_select.cc it is
  only used to close up temporary tables or during the process where a
  temporary table is converted over to being a myisam table.

  For sql_base.cc look at close_data_tables().

  @see
  sql_base.cc, sql_select.cc and table.cc
*/

int ha_spartan::close(void)
{
  DBUG_ENTER("ha_spartan::close");
  share->data_class->close_table();
  DBUG_RETURN(0);
}


/**
  @brief
  write_row() inserts a row. No extra() hint is given currently if a bulk load
  is happening. buf() is a byte array of data. You can use the field
  information to extract the data from the native byte array type.

  @details
  Example of this would be:
  @code
  for (Field **field=table->field ; *field ; field++)
  {
    ...
  }
  @endcode

  See ha_tina.cc for an spartan of extracting all of the data as strings.
  ha_berekly.cc has an spartan of how to store it intact by "packing" it
  for ha_berkeley's own native storage type.

  See the note for update_row() on auto_increments. This case also applies to
  write_row().

  Called from item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc, and sql_update.cc.

  @see
  item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc and sql_update.cc
*/

int ha_spartan::write_row(uchar *buf)
{
  DBUG_ENTER("ha_spartan::write_row");
  ha_statistic_increment(&SSV::ha_write_count);
  /*
    Begin critical section by locking the spartan mutex variable.
  */
  mysql_mutex_lock(&share->mutex);
  share->data_class->write_row(buf, table->s->rec_buff_length);
  /*
    End section by unlocking the spartan mutex variable.
  */
  mysql_mutex_unlock(&share->mutex);
  DBUG_RETURN(0);
}


/**
  @brief
  Yes, update_row() does what you expect, it updates a row. old_data will have
  the previous row record in it, while new_data will have the newest data in it.
  Keep in mind that the server can do updates based on ordering if an ORDER BY
  clause was used. Consecutive ordering is not guaranteed.

  @details
  Currently new_data will not have an updated auto_increament record. You can
  do this for spartan by doing:

  @code

  if (table->next_number_field && record == table->record[0])
    update_auto_increment();

  @endcode

  Called from sql_select.cc, sql_acl.cc, sql_update.cc, and sql_insert.cc.

  @see
  sql_select.cc, sql_acl.cc, sql_update.cc and sql_insert.cc
*/
int ha_spartan::update_row(const uchar *old_data, uchar *new_data)
{

  DBUG_ENTER("ha_spartan::update_row");
  /*
    Begin critical section by locking the spartan mutex variable.
  */
  mysql_mutex_lock(&share->mutex);
  share->data_class->update_row((uchar *)old_data, new_data, 
                 table->s->rec_buff_length, current_position -
                 share->data_class->row_size(table->s->rec_buff_length)); 
  /*
    End section by unlocking the spartan mutex variable.
  */
  mysql_mutex_unlock(&share->mutex);
  DBUG_RETURN(0);
}


/**
  @brief
  This will delete a row. buf will contain a copy of the row to be deleted.
  The server will call this right after the current row has been called (from
  either a previous rnd_nexT() or index call).

  @details
  If you keep a pointer to the last row or can access a primary key it will
  make doing the deletion quite a bit easier. Keep in mind that the server does
  not guarantee consecutive deletions. ORDER BY clauses can be used.

  Called in sql_acl.cc and sql_udf.cc to manage internal table
  information.  Called in sql_delete.cc, sql_insert.cc, and
  sql_select.cc. In sql_select it is used for removing duplicates
  while in insert it is used for REPLACE calls.

  @see
  sql_acl.cc, sql_udf.cc, sql_delete.cc, sql_insert.cc and sql_select.cc
*/

int ha_spartan::delete_row(const uchar *buf)
{
  DBUG_ENTER("ha_spartan::delete_row");
  long long pos;

  if (current_position > 0)
    pos = current_position -
      share->data_class->row_size(table->s->rec_buff_length);
  else
    pos = 0;
  /*
    Begin critical section by locking the spartan mutex variable.
  */
  mysql_mutex_lock(&share->mutex);
  share->data_class->delete_row((uchar *)buf, 
                                table->s->rec_buff_length, pos);
  /*
    End section by unlocking the spartan mutex variable.
  */
  mysql_mutex_unlock(&share->mutex);
  DBUG_RETURN(0);
}


/**
  @brief
  Positions an index cursor to the index specified in the handle. Fetches the
  row if available. If the key value is null, begin at the first key of the
  index.
*/

int ha_spartan::index_read_map(uchar *buf, const uchar *key,
                               key_part_map keypart_map __attribute__((unused)),
                               enum ha_rkey_function find_flag
                               __attribute__((unused)))
{
  int rc;
  DBUG_ENTER("ha_spartan::index_read");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  Used to read forward through the index.
*/

int ha_spartan::index_next(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_next");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  Used to read backwards through the index.
*/

int ha_spartan::index_prev(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_prev");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  index_first() asks for the first key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
*/
int ha_spartan::index_first(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_first");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  index_last() asks for the last key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
*/
int ha_spartan::index_last(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_last");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  rnd_init() is called when the system wants the storage engine to do a table
  scan. See the spartan in the introduction at the top of this file to see when
  rnd_init() is called.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc,
  and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and sql_update.cc
*/
int ha_spartan::rnd_init(bool scan)
{
  DBUG_ENTER("ha_spartan::rnd_init");
  current_position = 0;
  stats.records = 0;
  ref_length = sizeof(long long);
  DBUG_RETURN(0);
}

int ha_spartan::rnd_end()
{
  DBUG_ENTER("ha_spartan::rnd_end");
  DBUG_RETURN(0);
}


/**
  @brief
  This is called for each row of the table scan. When you run out of records
  you should return HA_ERR_END_OF_FILE. Fill buff up with the row information.
  The Field structure for the table is the key to getting data into buf
  in a manner that will allow the server to understand it.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc,
  and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and sql_update.cc
*/
int ha_spartan::rnd_next(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::rnd_next");
  MYSQL_READ_ROW_START(table_share->db.str, table_share->table_name.str,
                       TRUE);
  /*
    Read the row from the data file.
  */
  rc = share->data_class->read_row(buf, table->s->rec_buff_length,
                                   current_position); 
  if (rc != -1)
    current_position = (off_t)share->data_class->cur_position();
  else
    DBUG_RETURN(HA_ERR_END_OF_FILE);
  stats.records++;  
  MYSQL_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  position() is called after each call to rnd_next() if the data needs
  to be ordered. You can do something like the following to store
  the position:
  @code
  my_store_ptr(ref, ref_length, current_position);
  @endcode

  @details
  The server uses ref to store data. ref_length in the above case is
  the size needed to store current_position. ref is just a byte array
  that the server will maintain. If you are using offsets to mark rows, then
  current_position should be the offset. If it is a primary key like in
  BDB, then it needs to be a primary key.

  Called from filesort.cc, sql_select.cc, sql_delete.cc, and sql_update.cc.

  @see
  filesort.cc, sql_select.cc, sql_delete.cc and sql_update.cc
*/
void ha_spartan::position(const uchar *record)
{
  DBUG_ENTER("ha_spartan::position");
  my_store_ptr(ref, ref_length, current_position);
  DBUG_VOID_RETURN;
}


/**
  @brief
  This is like rnd_next, but you are given a position to use
  to determine the row. The position will be of the type that you stored in
  ref. You can use ha_get_ptr(pos,ref_length) to retrieve whatever key
  or position you saved when position() was called.

  @details
  Called from filesort.cc, records.cc, sql_insert.cc, sql_select.cc, and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_insert.cc, sql_select.cc and sql_update.cc
*/
int ha_spartan::rnd_pos(uchar *buf, uchar *pos)
{
  int rc;
  DBUG_ENTER("ha_spartan::rnd_pos");
  MYSQL_READ_ROW_START(table_share->db.str, table_share->table_name.str,
                       TRUE);
  ha_statistic_increment(&SSV::ha_read_rnd_next_count);
  current_position = (off_t)my_get_ptr(pos,ref_length);
  rc = share->data_class->read_row(buf, current_position, -1);
  MYSQL_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  ::info() is used to return information to the optimizer. See my_base.h for
  the complete description.

  @details
  Currently this table handler doesn't implement most of the fields really needed.
  SHOW also makes use of this data.

  You will probably want to have the following in your code:
  @code
  if (records < 2)
    records = 2;
  @endcode
  The reason is that the server will optimize for cases of only a single
  record. If, in a table scan, you don't know the number of records, it
  will probably be better to set records to two so you can return as many
  records as you need. Along with records, a few more variables you may wish
  to set are:
    records
    deleted
    data_file_length
    index_file_length
    delete_length
    check_time
  Take a look at the public variables in handler.h for more information.

  Called in filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc,
  sql_delete.cc, sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_show.cc,
  sql_table.cc, sql_union.cc, and sql_update.cc.

  @see
  filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc, sql_delete.cc,
  sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_table.cc,
  sql_union.cc and sql_update.cc
*/
int ha_spartan::info(uint flag)
{
  DBUG_ENTER("ha_spartan::info");
  /* This is a lie, but you don't want the optimizer to see zero or 1 */
  if (stats.records < 2)
    stats.records= 2;
  DBUG_RETURN(0);
}


/**
  @brief
  extra() is called whenever the server wishes to send a hint to
  the storage engine. The myisam engine implements the most hints.
  ha_innodb.cc has the most exhaustive list of these hints.

    @see
  ha_innodb.cc
*/
int ha_spartan::extra(enum ha_extra_function operation)
{
  DBUG_ENTER("ha_spartan::extra");
  DBUG_RETURN(0);
}


/**
  @brief
  Used to delete all rows in a table, including cases of truncate and cases where
  the optimizer realizes that all rows will be removed as a result of an SQL statement.

  @details
  Called from item_sum.cc by Item_func_group_concat::clear(),
  Item_sum_count_distinct::clear(), and Item_func_group_concat::clear().
  Called from sql_delete.cc by mysql_delete().
  Called from sql_select.cc by JOIN::reinit().
  Called from sql_union.cc by st_select_lex_unit::exec().

  @see
  Item_func_group_concat::clear(), Item_sum_count_distinct::clear() and
  Item_func_group_concat::clear() in item_sum.cc;
  mysql_delete() in sql_delete.cc;
  JOIN::reinit() in sql_select.cc and
  st_select_lex_unit::exec() in sql_union.cc.
*/
int ha_spartan::delete_all_rows()
{
  DBUG_ENTER("ha_spartan::delete_all_rows");
  /*
    Begin critical section by locking the spartan mutex variable.
  */
  mysql_mutex_lock(&share->mutex);
  share->data_class->trunc_table();
  /*
    End section by unlocking the spartan mutex variable.
  */
  mysql_mutex_unlock(&share->mutex);
  DBUG_RETURN(0);
}


/**
  @brief
  Used for handler specific truncate table.  The table is locked in
  exclusive mode and handler is responsible for reseting the auto-
  increment counter.

  @details
  Called from Truncate_statement::handler_truncate.
  Not used if the handlerton supports HTON_CAN_RECREATE, unless this
  engine can be used as a partition. In this case, it is invoked when
  a particular partition is to be truncated.

  @see
  Truncate_statement in sql_truncate.cc
  Remarks in handler::truncate.
*/
int ha_spartan::truncate()
{
  DBUG_ENTER("ha_spartan::truncate");
  delete_all_rows();
  DBUG_RETURN(0);
}


/**
  @brief
  This create a lock on the table. If you are implementing a storage engine
  that can handle transacations look at ha_berkely.cc to see how you will
  want to go about doing this. Otherwise you should consider calling flock()
  here. Hint: Read the section "locking functions for mysql" in lock.cc to understand
  this.

  @details
  Called from lock.cc by lock_external() and unlock_external(). Also called
  from sql_table.cc by copy_data_between_tables().

  @see
  lock.cc by lock_external() and unlock_external() in lock.cc;
  the section "locking functions for mysql" in lock.cc;
  copy_data_between_tables() in sql_table.cc.
*/
int ha_spartan::external_lock(THD *thd, int lock_type)
{
  DBUG_ENTER("ha_spartan::external_lock");
  DBUG_RETURN(0);
}


/**
  @brief
  The idea with handler::store_lock() is: The statement decides which locks
  should be needed for the table. For updates/deletes/inserts we get WRITE
  locks, for SELECT... we get read locks.

  @details
  Before adding the lock into the table lock handler (see thr_lock.c),
  mysqld calls store lock with the requested locks. Store lock can now
  modify a write lock to a read lock (or some other lock), ignore the
  lock (if we don't want to use MySQL table locks at all), or add locks
  for many tables (like we do when we are using a MERGE handler).

  Berkeley DB, for spartan, changes all WRITE locks to TL_WRITE_ALLOW_WRITE
  (which signals that we are doing WRITES, but are still allowing other
  readers and writers).

  When releasing locks, store_lock() is also called. In this case one
  usually doesn't have to do anything.

  In some exceptional cases MySQL may send a request for a TL_IGNORE;
  This means that we are requesting the same lock as last time and this
  should also be ignored. (This may happen when someone does a flush
  table when we have opened a part of the tables, in which case mysqld
  closes and reopens the tables and tries to get the same locks at last
  time). In the future we will probably try to remove this.

  Called from lock.cc by get_lock_data().

  @note
  In this method one should NEVER rely on table->in_use, it may, in fact,
  refer to a different thread! (this happens if get_lock_data() is called
  from mysql_lock_abort_for_thread() function)

  @see
  get_lock_data() in lock.cc
*/
THR_LOCK_DATA **ha_spartan::store_lock(THD *thd,
                                       THR_LOCK_DATA **to,
                                       enum thr_lock_type lock_type)
{
  if (lock_type != TL_IGNORE && lock.type == TL_UNLOCK)
    lock.type=lock_type;
  *to++= &lock;
  return to;
}


/**
  @brief
  Used to delete a table. By the time delete_table() has been called all
  opened references to this table will have been closed (and your globally
  shared references released). The variable name will just be the name of
  the table. You will need to remove any files you have created at this point.

  @details
  If you do not implement this, the default delete_table() is called from
  handler.cc and it will delete all files with the file extensions returned
  by bas_ext().

  Called from handler.cc by delete_table and ha_create_table(). Only used
  during create if the table_flag HA_DROP_BEFORE_CREATE was specified for
  the storage engine.

  @see
  delete_table and ha_create_table() in handler.cc
*/
int ha_spartan::delete_table(const char *name)
{
  DBUG_ENTER("ha_spartan::delete_table");
  char name_buff[FN_REFLEN];

  /*
    Call the mysql delete file method.
    Note: the fn_format() method correctly creates a file name from the
    name passed into the method.
  */
  my_delete(fn_format(name_buff, name, "", SDE_EXT,
            MY_REPLACE_EXT|MY_UNPACK_FILENAME), MYF(0));
  DBUG_RETURN(0);
}


/**
  @brief
  Renames a table from one name to another via an alter table call.

  @details
  If you do not implement this, the default rename_table() is called from
  handler.cc and it will delete all files with the file extensions returned
  by bas_ext().

  Called from sql_table.cc by mysql_rename_table().

  @see
  mysql_rename_table() in sql_table.cc
*/
int ha_spartan::rename_table(const char * from, const char * to)
{
  DBUG_ENTER("ha_spartan::rename_table ");
  char data_from[FN_REFLEN];
  char data_to[FN_REFLEN];

  my_copy(fn_format(data_from, from, "", SDE_EXT,
          MY_REPLACE_EXT|MY_UNPACK_FILENAME),
          fn_format(data_to, to, "", SDE_EXT,
          MY_REPLACE_EXT|MY_UNPACK_FILENAME), MYF(0));
  /*
    Delete the file using MySQL's delete file method.
  */
  my_delete(data_from, MYF(0));
  DBUG_RETURN(0);
}


/**
  @brief
  Given a starting key and an ending key, estimate the number of rows that
  will exist between the two keys.

  @details
  end_key may be empty, in which case determine if start_key matches any rows.

  Called from opt_range.cc by check_quick_keys().

  @see
  check_quick_keys() in opt_range.cc
*/
ha_rows ha_spartan::records_in_range(uint inx, key_range *min_key,
                                     key_range *max_key)
{
  DBUG_ENTER("ha_spartan::records_in_range");
  DBUG_RETURN(10);                         // low number to force index usage
}


/**
  @brief
  create() is called to create a database. The variable name will have the name
  of the table.

  @details
  When create() is called you do not need to worry about
  opening the table. Also, the .frm file will have already been
  created so adjusting create_info is not necessary. You can overwrite
  the .frm file at this point if you wish to change the table
  definition, but there are no methods currently provided for doing
  so.

  Called from handle.cc by ha_create_table().

  @see
  ha_create_table() in handle.cc
*/

int ha_spartan::create(const char *name, TABLE *table_arg,
                       HA_CREATE_INFO *create_info)
{
  DBUG_ENTER("ha_spartan::create");
  char name_buff[FN_REFLEN];

  if (!(share = get_share()))
    DBUG_RETURN(1);
  /*
    Call the data class create table method.
    Note: the fn_format() method correctly creates a file name from the
    name passed into the method.
  */
  if (share->data_class->create_table(fn_format(name_buff, name, "", SDE_EXT,
                                      MY_REPLACE_EXT|MY_UNPACK_FILENAME)))
    DBUG_RETURN(-1);
  share->data_class->close_table();
  DBUG_RETURN(0);
}

static ulong srv_enum_var= 0;
static ulong srv_ulong_var= 0;

const char *enum_var_names[]=
{
  "spartan_test1", "spartan_test2", NullS
};

TYPELIB enum_var_typelib=
{
  array_elements(enum_var_names) - 1, "enum_var_typelib",
  enum_var_names, NULL
};

static MYSQL_SYSVAR_ENUM(
  enum_var,                       // name
  srv_enum_var,                   // varname
  PLUGIN_VAR_RQCMDARG,            // opt
  "Sample ENUM system variable.", // comment
  NULL,                           // check
  NULL,                           // update
  0,                              // def
  &enum_var_typelib);             // typelib

static MYSQL_SYSVAR_ULONG(
  ulong_var,
  srv_ulong_var,
  PLUGIN_VAR_RQCMDARG,
  "0..1000",
  NULL,
  NULL,
  8,
  0,
  1000,
  0);

static struct st_mysql_sys_var* spartan_system_variables[]= {
  MYSQL_SYSVAR(enum_var),
  MYSQL_SYSVAR(ulong_var),
  NULL
};

// this is an spartan of SHOW_FUNC and of my_snprintf() service
static int show_func_spartan(MYSQL_THD thd, struct st_mysql_show_var *var,
                             char *buf)
{
  var->type= SHOW_CHAR;
  var->value= buf; // it's of SHOW_VAR_FUNC_BUFF_SIZE bytes
  my_snprintf(buf, SHOW_VAR_FUNC_BUFF_SIZE,
              "enum_var is %lu, ulong_var is %lu, %.6b", // %b is MySQL extension
              srv_enum_var, srv_ulong_var, "really");
  return 0;
}

static struct st_mysql_show_var func_status[]=
{
  {"spartan_func_spartan",  (char *)show_func_spartan, SHOW_FUNC},
  {0,0,SHOW_UNDEF}
};


struct st_mysql_storage_engine spartan_storage_engine=
{ MYSQL_HANDLERTON_INTERFACE_VERSION };

mysql_declare_plugin(spartan)
{
  MYSQL_STORAGE_ENGINE_PLUGIN,
  &spartan_storage_engine,
  "Spartan",
  "Chuck Bell",
  "Spartan Storage Engine Plugin",
  PLUGIN_LICENSE_GPL,
  spartan_init_func,                            /* Plugin Init */
  NULL,                                         /* Plugin Deinit */
  0x0100 /* 1.0 */,
  func_status,                                  /* status variables */
  spartan_system_variables,                     /* system variables */
  NULL,                                         /* config options */
  0,                                            /* flags */
}
mysql_declare_plugin_end;
