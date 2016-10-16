/*
  Spartan Storage Engine Plugin
*/

#include "my_global.h"                   /* ulonglong */
#include "thr_lock.h"                    /* THR_LOCK, THR_LOCK_DATA */
#include "handler.h"                     /* handler */
#include "spartan_data.h"

class Spartan_share : public Handler_share {
public:
  mysql_mutex_t mutex;
  THR_LOCK lock;
  Spartan_data *data_class;
  Spartan_share();
  ~Spartan_share()
  {
    thr_lock_delete(&lock);
    mysql_mutex_destroy(&mutex);
    if (data_class != NULL)
      delete data_class;
    data_class = NULL;
  }
};

/*
  Class definition for the storage engine
*/
class ha_spartan: public handler
{
  THR_LOCK_DATA lock;      /* MySQL lock */
  Spartan_share *share;    ///< Shared lock info
  Spartan_share *get_share(); ///< Get the share

public:
  ha_spartan(handlerton *hton, TABLE_SHARE *table_ar);
  ~ha_spartan()
  {
  }
  /* The name that will be used for display purposes */
  const char *table_type() const { return "SPARTAN"; }
  /*
    The name of the index type that will be used for display
    don't implement this method unless you really have indexes
   */
  const char *index_type(uint inx) { return "HASH"; }
  /*  
    The file extensions.
  */
  const char **bas_ext() const;
  /*
    This is a list of flags that says what the storage engine
    implements. The current table flags are documented in
    handler.h
  */
  ulonglong table_flags() const
  {
    return HA_BINLOG_STMT_CAPABLE;
  }
  /*
    This is a bitmap of flags that says how the storage engine
    implements indexes. The current index flags are documented in
    handler.h. If you do not implement indexes, just return zero
    here.

    part is the key part to check. First key part is 0
    If all_parts it's set, MySQL want to know the flags for the combined
    index up to and including 'part'.
  */
  ulong index_flags(uint inx, uint part, bool all_parts) const
  {
    return 0;
  }
  /*
    unireg.cc will call the following to make sure that the storage engine can
    handle the data it is about to send.

    Return *real* limits of your storage engine here. MySQL will do
    min(your_limits, MySQL_limits) automatically

    There is no need to implement ..._key_... methods if you don't suport
    indexes.
  */
  uint max_supported_record_length() const { return HA_MAX_REC_LENGTH; }
  uint max_supported_keys()          const { return 0; }
  uint max_supported_key_parts()     const { return 0; }
  uint max_supported_key_length()    const { return 0; }
  /*
    Called in test_quick_select to determine if indexes should be used.
  */
  virtual double scan_time() { return (double) (stats.records+stats.deleted) / 20.0+10; }
  /*
    The next method will never be called if you do not implement indexes.
  */
  virtual double read_time(uint, uint, ha_rows rows)
  { return (double) rows /  20.0+1; }

  /*
    Everything below are methods that we implment in ha_spartan.cc.

    Most of these methods are not obligatory, skip them and
    MySQL will treat them as not implemented
  */
  int open(const char *name, int mode, uint test_if_locked);    // required
  int close(void);                                              // required

  int write_row(uchar * buf);
  int update_row(const uchar * old_data, uchar * new_data);
  int delete_row(const uchar * buf);
  int index_read_map(uchar *buf, const uchar *key,
                     key_part_map keypart_map, enum ha_rkey_function find_flag);
  int index_next(uchar * buf);
  int index_prev(uchar * buf);
  int index_first(uchar * buf);
  int index_last(uchar * buf);
  /*
    unlike index_init(), rnd_init() can be called two times
    without rnd_end() in between (it only makes sense if scan=1).
    then the second call should prepare for the new table scan
    (e.g if rnd_init allocates the cursor, second call should
    position it to the start of the table, no need to deallocate
    and allocate it again
  */
  int rnd_init(bool scan);                                      //required
  int rnd_end();
  int rnd_next(uchar *buf);                                      //required
  int rnd_pos(uchar * buf, uchar *pos);                           //required
  void position(const uchar *record);                            //required
  int info(uint);                                              //required

  int extra(enum ha_extra_function operation);
  int external_lock(THD *thd, int lock_type);                   //required
  int delete_all_rows(void);
  int truncate();
  ha_rows records_in_range(uint inx, key_range *min_key,
                           key_range *max_key);
  int delete_table(const char *from);
  int rename_table(const char * from, const char * to);
  int create(const char *name, TABLE *form,
             HA_CREATE_INFO *create_info);                      //required

  THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                             enum thr_lock_type lock_type);     //required
};

