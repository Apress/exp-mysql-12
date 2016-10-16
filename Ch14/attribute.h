#include "sql_priv.h"
#include "sql_class.h"
#include "table.h"

class Attribute
{
public:
  Attribute(void);
  int remove_attribute(int num);
  Item *get_attribute(int num);
  int add_attribute(bool append, Item *new_item);
  int num_attributes();
  int index_of(char *table, char *value);
  int hide_attribute(Item *item, bool hide);
  char *to_string();
private:
  List<Item> attr_list;
  bool hidden[256];
};
