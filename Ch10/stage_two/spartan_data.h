/*
  Spartan_data.h

  This header defines a simple data file class for reading raw data to and 
  from disk. The data written is in byte format so it can be anything you 
  want it to be. The write_row and read_row accept the length of the data 
  item to be read.
*/
#include "my_global.h"
#include "my_sys.h"

class Spartan_data
{
public:
  Spartan_data(void);
  ~Spartan_data(void);
  int create_table(char *path);
  int open_table(char *path);
  long long write_row(uchar *buf, int length);
  long long update_row(uchar *old_rec, uchar *new_rec,
                       int length, long long position);
  int read_row(uchar *buf, int length, long long position);
  int delete_row(uchar *old_rec, int length, long long position);
  int close_table();
  long long cur_position();
  int records();
  int del_records();
  int trunc_table();
  int row_size(int length);
private:
  File data_file;
  int header_size;
  int record_header_size;
  bool crashed;
  int number_records;
  int number_del_records;
  int read_header();
  int write_header();
};
