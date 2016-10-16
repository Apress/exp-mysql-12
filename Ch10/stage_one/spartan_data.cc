/*
  Spartan_data.cc

  This class implements a simple data file reader/writer. It
  is designed to allow the caller to specify the size of the
  data to read or write. This allows for variable length records
  and the inclusion of extra fields (like blobs). The data is
  store in an uncompressed, unoptimized fashion.
*/
#include "spartan_data.h"
#include <my_dir.h>
#include <string.h>

Spartan_data::Spartan_data(void)
{
  data_file = -1;
  number_records = -1;
  number_del_records = -1;
  header_size = sizeof(bool) + sizeof(int) + sizeof(int);
  record_header_size = sizeof(uchar) + sizeof(int);
}

Spartan_data::~Spartan_data(void)
{
}

/* create the data file */
int Spartan_data::create_table(char *path)
{
  DBUG_ENTER("SpartanIndex::create_table");
  open_table(path);
  number_records = 0;
  number_del_records = 0;
  crashed = false;
  write_header();  
  DBUG_RETURN(0);
}

/* open table at location "path" = path + filename */
int Spartan_data::open_table(char *path)
{
  DBUG_ENTER("Spartan_data::open_table");
  /*
    Open the file with read/write mode,
    create the file if not found, 
    treat file as binary, and use default flags.
  */
  data_file = my_open(path, O_RDWR | O_CREAT | O_BINARY | O_SHARE, MYF(0));
  if(data_file == -1)
    DBUG_RETURN(errno);
  read_header();
  DBUG_RETURN(0);
}

/* write a row of length bytes to file and return position */
long long Spartan_data::write_row(uchar *buf, int length)
{
  long long pos;
  int i;
  int len;
  uchar deleted = 0;

  DBUG_ENTER("Spartan_data::write_row");
  /*
    Write the deleted status byte and the length of the record.
    Note: my_write() returns the bytes written or -1 on error
  */
  pos = my_seek(data_file, 0L, MY_SEEK_END, MYF(0));
  /*
    Note: my_malloc takes a size of memory to be allocated,
    MySQL flags (set to zero fill and with extra error checking).
    Returns number of bytes allocated -- <= 0 indicates an error.
  */
  i = my_write(data_file, &deleted, sizeof(uchar), MYF(0));
  memcpy(&len, &length, sizeof(int));
  i = my_write(data_file, (uchar *)&len, sizeof(int), MYF(0));
  /*
    Write the row data to the file. Return new file pointer or
    return -1 if error from my_write().
  */
  i = my_write(data_file, buf, length, MYF(0));
  if (i == -1)
    pos = i; 
  else
    number_records++;
  DBUG_RETURN(pos);
}

/* update a record in place */
long long Spartan_data::update_row(uchar *old_rec, uchar *new_rec,
                                   int length, long long position)
{
  long long pos;
  long long cur_pos;
  uchar *cmp_rec;
  int len;
  uchar deleted = 0;
  int i = -1;  
  
  DBUG_ENTER("Spartan_data::update_row");
  if (position == 0)
    position = header_size; //move past header
  pos = position;
  /* 
    If position unknown, scan for the record by reading a row
    at a time until found.
  */
  if (position == -1) //don't know where it is...scan for it
  {
    cmp_rec = (uchar *)my_malloc(length, MYF(MY_ZEROFILL | MY_WME));
    pos = 0;
    /*
      Note: my_seek() returns pos if no errors or -1 if error.
    */
    cur_pos = my_seek(data_file, header_size, MY_SEEK_SET, MYF(0));
    /* 
      Note: read_row() returns current file pointer if no error or
      -1 if error.
    */
    while ((cur_pos != -1) && (pos != -1))
    {
      pos = read_row(cmp_rec, length, cur_pos);
      if (memcmp(old_rec, cmp_rec, length) == 0)
      {
        pos = cur_pos;      //found it!
        cur_pos = -1;       //stop loop gracefully
      }
      else if (pos != -1)   //move ahead to next rec
        cur_pos = cur_pos + length + record_header_size;
    }
    my_free(cmp_rec);
  }
  /*
    If position found or provided, write the row.
  */
  if (pos != -1)
  {
    /*
      Write the deleted byte, the length of the row, and the data
      at the current file pointer.
      Note: my_write() returns the bytes written or -1 on error
    */
    my_seek(data_file, pos, MY_SEEK_SET, MYF(0));
    i = my_write(data_file, &deleted, sizeof(uchar), MYF(0));
    memcpy(&len, &length, sizeof(int));
    i = my_write(data_file, (uchar *)&len, sizeof(int), MYF(0));
    pos = i;
    i = my_write(data_file, new_rec, length, MYF(0));
  }
  DBUG_RETURN(pos);
}

/* delete a record in place */
int Spartan_data::delete_row(uchar *old_rec, int length, 
                             long long position)
{
  int i = -1;
  long long pos;
  long long cur_pos;
  uchar *cmp_rec;
  uchar deleted = 1;
  
  DBUG_ENTER("Spartan_data::delete_row");
  if (position == 0)
    position = header_size; //move past header
  pos = position;
  /* 
    If position unknown, scan for the record by reading a row
    at a time until found.
  */
  if (position == -1) //don't know where it is...scan for it
  {
    cmp_rec = (uchar *)my_malloc(length, MYF(MY_ZEROFILL | MY_WME));
    pos = 0;
    /*
      Note: my_seek() returns pos if no errors or -1 if error.
    */
    cur_pos = my_seek(data_file, header_size, MY_SEEK_SET, MYF(0));
    /* 
      Note: read_row() returns current file pointer if no error or
      -1 if error.
    */
    while ((cur_pos != -1) && (pos != -1))
    {
      pos = read_row(cmp_rec, length, cur_pos);
      if (memcmp(old_rec, cmp_rec, length) == 0)
      {
        number_records--;
        number_del_records++;
        pos = cur_pos;
        cur_pos = -1;
      }
      else if (pos != -1)   //move ahead to next rec
        cur_pos = cur_pos + length + record_header_size;
    }
    my_free(cmp_rec);
  }
  /*
    If position found or provided, write the row.
  */
  if (pos != -1)            //mark as deleted
  {
    /*
      Write the deleted byte set to 1 which marks row as deleted
      at the current file pointer.
      Note: my_write() returns the bytes written or -1 on error
    */
    pos = my_seek(data_file, pos, MY_SEEK_SET, MYF(0));
    i = my_write(data_file, &deleted, sizeof(uchar), MYF(0));
    i = (i > 1) ? 0 : i;
  }
  DBUG_RETURN(i);
}

/* read a row of length bytes from file at position */
int Spartan_data::read_row(uchar *buf, int length, long long position)
{
  int i;
  int rec_len;
  long long pos; 
  uchar deleted = 2;

  DBUG_ENTER("Spartan_data::read_row");
  if (position <= 0)
    position = header_size; //move past header
  pos = my_seek(data_file, position, MY_SEEK_SET, MYF(0));
  /*
    If my_seek found the position, read the deleted byte.
    Note: my_read() returns bytes read or -1 on error
  */
  if (pos != -1L)
  {
    i = my_read(data_file, &deleted, sizeof(uchar), MYF(0));
    /*
      If not deleted (deleted == 0), read the record length then
      read the row.
    */
    if (deleted == 0) /* 0 = not deleted, 1 = deleted */
    {
      i = my_read(data_file, (uchar *)&rec_len, sizeof(int), MYF(0));
      i = my_read(data_file, buf, 
                 (length < rec_len) ? length : rec_len, MYF(0));
    }
    else if (i == 0)
      DBUG_RETURN(-1);      
    else
      DBUG_RETURN(read_row(buf, length, cur_position() + 
                           length + (record_header_size - sizeof(uchar))));
  }
  else
    DBUG_RETURN(-1);
  DBUG_RETURN(0);
}

/* close file */
int Spartan_data::close_table()
{
  DBUG_ENTER("Spartan_data::close_table");
  if (data_file != -1)
  {
    my_close(data_file, MYF(0));
    data_file = -1;
  }
  DBUG_RETURN(0);
}

/* return number of records */
int Spartan_data::records()
{
  DBUG_ENTER("Spartan_data::num_records");
  DBUG_RETURN(number_records);
}

/* return number of deleted records */
int Spartan_data::del_records()
{
  DBUG_ENTER("Spartan_data::num_records");
  DBUG_RETURN(number_del_records);
}

/* read header from file */
int Spartan_data::read_header()
{
  int i;
  int len;

  DBUG_ENTER("Spartan_data::read_header");
  if (number_records == -1)
  {  
    my_seek(data_file, 0l, MY_SEEK_SET, MYF(0));
    i = my_read(data_file, (uchar *)&crashed, sizeof(bool), MYF(0));
    i = my_read(data_file, (uchar *)&len, sizeof(int), MYF(0));
    memcpy(&number_records, &len, sizeof(int));
    i = my_read(data_file, (uchar *)&len, sizeof(int), MYF(0));
    memcpy(&number_del_records, &len, sizeof(int));
  }
  else
    my_seek(data_file, header_size, MY_SEEK_SET, MYF(0));
  DBUG_RETURN(0);
}

/* write header to file */
int Spartan_data::write_header()
{
  int i;

  DBUG_ENTER("Spartan_data::write_header");
  if (number_records != -1)
  {
    my_seek(data_file, 0l, MY_SEEK_SET, MYF(0));
    i = my_write(data_file, (uchar *)&crashed, sizeof(bool), MYF(0));
    i = my_write(data_file, (uchar *)&number_records, sizeof(int), MYF(0));
    i = my_write(data_file, (uchar *)&number_del_records, sizeof(int), MYF(0));
  }
  DBUG_RETURN(0);
}

/* get position of the data file */
long long Spartan_data::cur_position()
{
  long long pos;

  DBUG_ENTER("Spartan_data::cur_position");
  pos = my_seek(data_file, 0L, MY_SEEK_CUR, MYF(0));
  if (pos == 0)
    DBUG_RETURN(header_size);
  DBUG_RETURN(pos);
}

/* truncate the data file */
int Spartan_data::trunc_table()
{
  DBUG_ENTER("Spartan_data::trunc_table");
  if (data_file != -1 )
  {
    my_chsize(data_file, 0, 0, MYF(MY_WME));
    write_header();
  }
  DBUG_RETURN(0);
}

/* determine the row size of the data file */
int Spartan_data::row_size(int length)
{
  DBUG_ENTER("Spartan_data::row_size");
  DBUG_RETURN(length + record_header_size);
}
