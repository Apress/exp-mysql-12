/*
  expression.cc

  DESCRIPTION
    This file contains methods to encapsulate the expressions in a query.

  SEE ALSO
    expression.h
*/ 
#include "expression.h"

/*
  Constructor

  SYNOPSIS
    Expression()

  DESCRIPTION
    Initializes the private variables.
*/
Expression::Expression(void) 
{
  root = NULL;
  num_expr = 0;
}

/*
  Remove Expression

  SYNOPSIS
    remove_expression()
    int num IN the number (starting at 0) of the sttribute to drop.

  DESCRIPTION
    This method removes the expression from the collection.

  RETURN VALUE
    Success = true
    Failed = false
*/
int Expression::remove_expression(int num, bool free)
{
  expr_node *p = root;
  expr_node *q = NULL;
  int j = 0;

  DBUG_ENTER("remove_expression");
  if (num >= num_expr || num < 0)
    DBUG_RETURN(false);
  while ((j != num) && (p != NULL))
  {
    q = p;
    p = p->next;
    j++;
  }
  if (j == 0)
    root = root->next;
  else if (j == num)
  {
    if (q != NULL)
      q->next = p->next;
    else
      root = NULL;
  }
  num_expr--;
  if (free)
    my_free(p);
  DBUG_RETURN(true);
}

/*
  Get Expression

  SYNOPSIS
    get_expression()
    int num IN the number of expression to retrieve.

  DESCRIPTION
    This method returns the expression at the index specified.

  RETURN VALUE
    Success = expr_node *
    Failed = NULL
*/
expr_node *Expression::get_expression(int num)
{
  int j = 0;
  expr_node *p = root;

  DBUG_ENTER("get_expression");
  if (num >= num_expr || num < 0)
    DBUG_RETURN(NULL);
  while ((j != num) && (p != NULL))
  {
    p = p->next;
    j++;
  }
  if (j == num)
    DBUG_RETURN(p);
  else
    DBUG_RETURN(NULL);
}

/*
  Add Expression

  SYNOPSIS
    add_expression()
    bool append IN do you want to append (true) or prepend (false)?
    expr_node *new_item IN the item to be added.

  DESCRIPTION
    This method adds the expression at the front or back of the list.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Expression::add_expression(bool append, expr_node *new_item)
{
  expr_node *p = root;

  DBUG_ENTER("add_expression");
  if (append)
    if (root == NULL)
      root = new_item;
    else
    {
      while (p->next != NULL)
        p = p->next;
      if (p == root)
        root->next = new_item;
      else
        p->next = new_item;
      new_item->next = NULL;
    }
  else
  {
    root = new_item;
    root->next = p;
  }
  num_expr++;
  DBUG_RETURN(0);
}

/*
  Number of Expressions

  SYNOPSIS
    num_expressions()

  DESCRIPTION
    This method returns the number of expressions in the list.

  RETURN VALUE
    int Number of expressions in list
*/
int Expression::num_expressions()
{
  DBUG_ENTER("num_expressions");
  DBUG_RETURN(num_expr);
}

/*
  Find index of Expression

  SYNOPSIS
    index_of()
    char *table IN table name of the expression
    char *value IN the name of the expression

  DESCRIPTION
    This method returns the index (number) of expression specified.

  RETURN VALUE
    Success = int Number of expression
    Failed = -1
*/
int Expression::index_of(char *table, char *value)
{
  expr_node *p = root;
  bool found = false;
  int i = 0;

  DBUG_ENTER("index_of");
  while ((p != NULL) && !found)
  {
    i++;
    if ((strcasecmp(((Item_field *)p->left_op)->field_name, value) == 0) &&
       ((table == NULL) ||
       (strcasecmp(((Item_field *)p->left_op)->table_name, table) == 0)))
      found = true;
    else if ((strcasecmp(((Item_field *)p->right_op)->field_name, value) == 0) &&
       ((table == NULL) ||
       (strcasecmp(((Item_field *)p->right_op)->table_name, table) == 0)))
      found = true;
    else 
      p = p->next;
  }
  DBUG_RETURN(i);
}

/*
  Does the expression list have the table?

  SYNOPSIS
    has_table()
    char * IN The name of the table.

  DESCRIPTION
    This method searches the expression list to see if the table passed in
    is referenced in the list of expressions.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Expression::has_table(char *table)
{
  expr_node *p = root;
  bool found = false;
  int i;
  int j;

  DBUG_ENTER("has_table");
  while ((p != NULL) && !found)
  {
    if (p->left_op->type() == Item::FIELD_ITEM)
      i = strcasecmp((char *)((Item_field *)p->left_op)->table_name, 
                     table);
    else
      i = -1;
    if (p->right_op->type() == Item::FIELD_ITEM)
      j = strcasecmp((char *)((Item_field *)p->right_op)->table_name,
                     table);
    else
      j = -1;
    if ((i == 0) || (j == 0))
      found = true;
    else 
      p = p->next;
  }
  DBUG_RETURN(found);
}

/*
  Reduce expressions to minimal expression.

  SYNOPSIS
    reduce_expressions()
    TABLE * IN The table that the expression references

  DESCRIPTION
    This method eliminates "dead" or always true expressions from the list.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Expression::reduce_expressions(TABLE *table)
{
  DBUG_ENTER("reduce_expressions");
  DBUG_RETURN(0);
}

/*
  Convert LEX structure to Expression class.

  SYNOPSIS
    convert()
    Item * IN The lex structure to convert.

  DESCRIPTION
    This method parses a lex->where or lex->on_expr to the Expression class
    adding the expressions to the list.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Expression::convert(THD *thd, Item *mysql_expr)
{
  Item *pcond = mysql_expr;

  DBUG_ENTER("convert");
  if (pcond == NULL)
    DBUG_RETURN(0);
  while (pcond != NULL)
  {
    expr_node *enode = (expr_node *)my_malloc(sizeof(expr_node), 
                                              MYF(MY_ZEROFILL | MY_WME));
    if (pcond->type() == Item::COND_ITEM)
    {
      enode->junction = pcond;
      pcond = pcond->next;  
    }
    else
      enode->junction = NULL;
    enode->operation = pcond;
    pcond = pcond->next;
    if (pcond == NULL)
      break;
    if (pcond->type() == Item::FIELD_ITEM)
    {
      if (!pcond->fixed)
        pcond->fix_fields(thd, &pcond);
    }
    enode->right_op = pcond;
    pcond = pcond->next;
    if (pcond == NULL)
      break;
    if (pcond->type() == Item::FIELD_ITEM)
    {
      if (!pcond->fixed)
        pcond->fix_fields(thd, &pcond);
    }
    enode->left_op = pcond;
    enode->next = NULL;
    add_expression(true, enode);
    pcond = pcond->next;
    if (pcond != NULL)
      if (pcond->next == NULL)
        pcond = NULL;
  }
  DBUG_RETURN(0);
}

/*
  Get string value of expressions

  SYNOPSIS
    to_string()

  DESCRIPTION
    This method returns a formatted string of the expressions in the list.

  RETURN VALUE
    Success = char *
    Failed = ""
*/
char *Expression::to_string()
{
  char *str = (char *)my_malloc(1024, MYF(MY_ZEROFILL | MY_WME));
  Item *pcond;

  DBUG_ENTER("to_string");
  strcpy(str, "");
  if (root == NULL)
    DBUG_RETURN((char *)"<none>");
  pcond = root->left_op;
  while (pcond->next)
  {
    strcat(str, " (");
    if (pcond->type() == Item::FIELD_ITEM)
    {
      strcat(str, ((Item_field *)pcond)->field_name);
    }
    else
    {
      strcat(str, "COND_ITEM");
    }
    strcat(str, ")");
    pcond = pcond->next;
  }
  DBUG_RETURN(str);
}

/*
  Get field in table.

  SYNOPSIS
    find_field()
    TABLE *table IN the table to search
    char *name IN the field name to search for

  DESCRIPTION
    This method returns the field class that matches the field name.

  RETURN VALUE
    Success = Field *
    Failed = NULL
*/
Field *Expression::find_field(TABLE *tbl, char *name)
{
  DBUG_ENTER("find_field");
  for (Field **field = tbl->field; *field; field++)
  {
    if (strcasecmp((*field)->field_name, name) == 0)
      DBUG_RETURN((*field));
  }
  DBUG_RETURN(NULL);
}

/*
  Compare join values.

  SYNOPSIS
    compare_join()
    expr_node *expr IN the expression to evaluate
    TABLE *t1 IN the left table to compare
    TABLE *t2 IN the left table to compare

  DESCRIPTION
    This method compares the values and returns the result

  RETURN VALUE
    Success = true
    Failed = false
*/
int Expression::compare_join(expr_node *expr, TABLE *t1, TABLE *t2)
{
  char buff1[MAX_FIELD_WIDTH];
  char buff2[MAX_FIELD_WIDTH];
  String str1(buff1,sizeof(buff1),&my_charset_bin);
  String str2(buff2,sizeof(buff2),&my_charset_bin);

  DBUG_ENTER("compare_join");
  if (expr == NULL)
    DBUG_RETURN(-1);
  if ((expr->left_op == NULL) || (expr->right_op == NULL) ||
      (expr->operation == NULL))
    DBUG_RETURN(-1);
  /*
    If both are fields, compare the field values. (join)
  */
  if ((expr->left_op->type() == Item::FIELD_ITEM) &&
     (expr->right_op->type() == Item::FIELD_ITEM))
  {
    int i;
    Field *fl = find_field(t1, (char *)((Item_field *)expr->left_op)->field_name);
    fl->val_str(&str1);
    int fl_len = str1.length();
    Field *fr = find_field(t2, (char *)((Item_field *)expr->right_op)->field_name);
    fr->val_str(&str2);
    int fr_len = str2.length();
    
    i = memcmp(str1.ptr(), str2.ptr(), fl_len < fr_len ? fl_len : fr_len);
    DBUG_RETURN(i);
  }
  DBUG_RETURN(90125);
}

/*
  Compare values.

  SYNOPSIS
    compare()
    expr_node *expr IN the expression to evaluate
    TABLE *t1 IN the left table to compare

  DESCRIPTION
    This method compares the values and returns the result

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Expression::compare(expr_node *expr, TABLE *t1)
{
  bool result = false;

  DBUG_ENTER("compare");
  if (expr == NULL)
    DBUG_RETURN(-1);
  if ((expr->left_op == NULL) || (expr->right_op == NULL) ||
      (expr->operation == NULL))
    DBUG_RETURN(-1);
  /*
    Check for compare integers.
  */
  if ((expr->left_op->type() == Item::INT_ITEM) ||
      (expr->right_op->type() == Item::INT_ITEM))
  {
    longlong a = 0;
    longlong b = 0;
    Field *f;

    if(expr->left_op->type() == Item::FIELD_ITEM)
    {
      f = find_field(t1, (char *)((Item_field *)expr->left_op)->field_name);
      if (f == NULL)
        DBUG_RETURN(-1);
      a = f->val_int();
    }
    else
      a = atol(expr->left_op->item_name.ptr());
    if(expr->right_op->type() == Item::FIELD_ITEM)
    {
      f = find_field(t1, (char *)((Item_field *)expr->right_op)->field_name);
      if (f == NULL)
        DBUG_RETURN(-1);
      b = f->val_int();
    }
    else
      b = atol(expr->right_op->item_name.ptr());
    switch(((Item_func *)expr->operation)->functype())
    {
      case Item_func::EQ_FUNC:
        DBUG_RETURN(a == b);
      case Item_func::NE_FUNC:
        DBUG_RETURN(a != b);
      case Item_func::LT_FUNC:
        DBUG_RETURN(a < b);
      case Item_func::LE_FUNC:
        DBUG_RETURN(a <= b);
      case Item_func::GT_FUNC:
        DBUG_RETURN(a > b);
      case Item_func::GE_FUNC:
        DBUG_RETURN(a >= b);
      default:
        DBUG_RETURN(false);
    }
  }
  /*
    Check for compare strings.
  */
  else if ((expr->left_op->type() == Item::STRING_ITEM) ||
           (expr->right_op->type() == Item::STRING_ITEM))
  {
    char buff1[MAX_FIELD_WIDTH];
    char buff2[MAX_FIELD_WIDTH];
    String str1(buff1,sizeof(buff1),&my_charset_bin);
    String str2(buff2,sizeof(buff2),&my_charset_bin);
    char *a = NULL;
    char *b = NULL;
    int i, l, r;
    Field *f;

    if(expr->left_op->type() == Item::FIELD_ITEM)
    {
      f = find_field(t1, (char *)((Item_field *)expr->left_op)->field_name);
      if (f == NULL)
        DBUG_RETURN(-1);
      f->val_str(&str1);
      a = (char *)str1.ptr();
      l = str1.length();
    }
    else
    {
      a = (char *)expr->left_op->item_name.ptr();
      l = strlen(a);
    }
    if(expr->right_op->type() == Item::FIELD_ITEM)
    {
      f = find_field(t1, (char *)((Item_field *)expr->right_op)->field_name);
      if (f == NULL)
        DBUG_RETURN(-1);
      f->val_str(&str2);
      b = (char *)str2.ptr();
      r = str2.length();
    }
    else
    {
      b = (char *)expr->right_op->item_name.ptr();
      r = strlen(b);
    }
    i = strncasecmp(a, b, l < r ? l : r);
    switch(((Item_func *)expr->operation)->functype())
    {
      case Item_func::EQ_FUNC:
        DBUG_RETURN(i == 0);
      case Item_func::NE_FUNC:
        DBUG_RETURN(i != 0);
      case Item_func::LT_FUNC:
        DBUG_RETURN(i < 0);
      case Item_func::LE_FUNC:
        DBUG_RETURN(i <= 0);
      case Item_func::GT_FUNC:
        DBUG_RETURN(i > 0);
      case Item_func::GE_FUNC:
        DBUG_RETURN(i >= 0);
      default:
        DBUG_RETURN(false);
    }
  }
  /*
    Check for compare decimal.
  */
  else if ((expr->left_op->type() == Item::DECIMAL_ITEM) ||
           (expr->right_op->type() == Item::DECIMAL_ITEM))
  {
    my_decimal *a = new my_decimal();
    my_decimal *b = new my_decimal();
    Field *f;

    if(expr->left_op->type() == Item::FIELD_ITEM)
    {
      f = find_field(t1, (char *)((Item_field *)expr->left_op)->field_name);
      if (f == NULL)
        DBUG_RETURN(-1);
      a = f->val_decimal(a);
    }
    else
      str2my_decimal(E_DEC_FATAL_ERROR, expr->left_op->item_name.ptr(), 
        expr->left_op->item_name.length(), system_charset_info, a);
    if(expr->right_op->type() == Item::FIELD_ITEM)
    {
      f = find_field(t1, (char *)((Item_field *)expr->right_op)->field_name);
      if (f == NULL)
        DBUG_RETURN(-1);
      b = f->val_decimal(b);
    }
    else
      str2my_decimal(E_DEC_FATAL_ERROR, expr->right_op->item_name.ptr(), 
        expr->right_op->item_name.length(), system_charset_info, b);
    a->fix_buffer_pointer();
    b->fix_buffer_pointer();
    result = (decimal_cmp((decimal_t*) a, (decimal_t*) b) == 0);
    DBUG_RETURN (result);
  }
  DBUG_RETURN(result);
}

/*
  Evaluation expression conditions.

  SYNOPSIS
    evaluate()
    TABLE *table1 IN the left table to compare

  DESCRIPTION
    This method evalutates the expression and returns the result

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Expression::evaluate(TABLE *table1)
{
  bool result = false;
  expr_node *node = root;
  Item_func *junction = NULL;

  DBUG_ENTER("evaluate");
  while (node != NULL)
  {
    if (node == root)
    {
      result = compare(node, table1);
      if (node->junction != NULL)
        junction = (Item_func *)node->junction;
      else
        junction = NULL;
    }
    else
      switch (junction->functype())
      {
         case Item_func::COND_AND_FUNC:
          result = result && compare(node, table1);
          break;
        case Item_func::COND_OR_FUNC:
          result = result || compare(node, table1);
          break;
        default:
          DBUG_RETURN(false);
      }
    node = node->next;
  }
  DBUG_RETURN (result);
}

int Expression::get_join_expr(Expression *where_expr)
{
  bool result = false;
  expr_node *node;
  int i = 0;

  DBUG_ENTER("get_join_expr");
  node = where_expr->get_expression(i);
  while (node != NULL)
  {
    if ((node->left_op->type() == Item::FIELD_ITEM) &&
        (node->right_op->type() == Item::FIELD_ITEM))
    {
      where_expr->remove_expression(i, false);
      node->next = NULL;
      add_expression(true, node);
      i--;
    }
    i++;
    node = where_expr->get_expression(i);
  }
  DBUG_RETURN (result);
}
