/*
  attribute.cc

  DESCRIPTION
    This file contains methods to encapsulate the attributes in a query.

  SEE ALSO
    attribute.h
*/
#include "attribute.h"

/*
  Constructor

  SYNOPSIS
    Attribute()

  DESCRIPTION
    Initializes the hidden attribute array.
*/
Attribute::Attribute(void)
{
  int i;
  for (i = 0; i < 256; i++)
    hidden[i] = false;
}

/*
  Remove Attribute

  SYNOPSIS
    remove_attribute()
    int num IN the number (starting at 0) of the sttribute to drop.

  DESCRIPTION
    This method removes the attribute from the collection.

  RETURN VALUE
    Success = true
    Failed = false
*/
int Attribute::remove_attribute(int num)
{
  int j = 0;
  List_iterator<Item> it(attr_list);
  Item *item = NULL;

  DBUG_ENTER("remove_attribute");
  if (num >= (int)attr_list.elements || num < 0)
    DBUG_RETURN(false);
  while ((item = it++))
  {
    if (j == num)
    {
      it.remove();
      DBUG_RETURN(true);
    }
    j++;
  }
  DBUG_RETURN(false);
}

/*
  Get Attribute

  SYNOPSIS
    get_attribute()
    int num IN the number of attribute to retrieve.

  DESCRIPTION
    This method returns the attribute at the index specified.

  RETURN VALUE
    Success = Item *
    Failed = NULL
*/
Item *Attribute::get_attribute(int num)
{
  int j = 0;
  List<Item> it(attr_list);
  Item *item = it.pop();

  DBUG_ENTER("get_attribute");
  if (num >= (int)attr_list.elements || num < 0)
    DBUG_RETURN(NULL);
  while ((j != num) && (item != NULL))
  {
    item = it.pop();
    j++;
  }
  if (j != num)
    item = 0;
  DBUG_RETURN(item);
}

/*
  Add Attribute

  SYNOPSIS
    add_attribute()
    bool append IN do you want to append (true) or prepend (false)?
    Item *new_item IN the item to be added.

  DESCRIPTION
    This method adds the attribute at the front or back of the list.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Attribute::add_attribute(bool append, Item *new_item)
{
  DBUG_ENTER("add_attribute");
  if (append)
    attr_list.push_back(new_item);
  else
    attr_list.push_front(new_item);
  DBUG_RETURN(0);
}

/*
  Number of Attributes

  SYNOPSIS
    num_attributes()

  DESCRIPTION
    This method returns the number of attributes in the list.

  RETURN VALUE
    int Number of attributes in list
*/
int Attribute::num_attributes()
{
  DBUG_ENTER("num_attributes");
  DBUG_RETURN(attr_list.elements);
}

/*
  Find index of Attribute

  SYNOPSIS
    index_of()
    char *table IN table name of the attribute
    char *value IN the name of the attribute

  DESCRIPTION
    This method returns the index (number) of attribute specified.

  RETURN VALUE
    Success = int Number of attribute
    Failed = -1
*/
int Attribute::index_of(char *table, char *value)
{
  List_iterator<Item> it(attr_list);
  Item *item;
  int i = 0;
  int j = -1;
  bool found = false;

  DBUG_ENTER("index_of");
  while ((item = it++) && !found)
  {
    if ((strcasecmp(((Field *)item)->field_name, value) == 0) &&
       ((table == NULL) ||
       (strcasecmp(*((Field *)item)->table_name, table) == 0)))
    {
      j = i;
      found = true;
    }
    i++;
  }
  DBUG_RETURN(j);
}

/*
  Hide Attribute

  SYNOPSIS
    hide_attribute()
    int num IN the number of attribute to hide.

  DESCRIPTION
    This method sets the hidden property of the attribute to specify
    that the attribute is not to be sent to the client (used internally)
    by the optimizer.

  RETURN VALUE
    Success = 0
    Failed = -1
*/
int Attribute::hide_attribute(Item *item, bool hide)
{
  int i;
  DBUG_ENTER("hide_attribute");
  i = index_of((char *)*((Field *)item)->table_name, 
               (char *)((Field *)item)->field_name);
  if (i > -1)
    hidden[i] = hide;
  DBUG_RETURN(0);
}

/*
  Get string value of attributes

  SYNOPSIS
    to_string()

  DESCRIPTION
    This method returns a formatted string of the attributes in the list.

  RETURN VALUE
    Success = char *
    Failed = ""
*/
char *Attribute::to_string()
{
  List_iterator<Item> it(attr_list);
  char *str = (char *)my_malloc(1024, MYF(MY_ZEROFILL | MY_WME));
  Item *item;
  int j = 0;

  DBUG_ENTER("to_string");
  strcpy(str, "");
  while ((item = it++))
  {
    if (j)
      strcat(str, ", ");
    if (*((Field *)item)->table_name)
    {
      strcat(str, *((Field *)item)->table_name);
      strcat(str, ".");
    }
    strcat(str, ((Field *)item)->field_name);
    if (!j)
      j = 1;
  }
  DBUG_RETURN(str);
}
