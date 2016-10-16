/*
  sql_dbxp_parse.cc

  DESCRIPTION
    This file contains methods to execute the DBXP SELECT query statements.

  NOTES
    The data structure is a binary tree that can have 0, 1, or 2 children. Only
    Join operations can have 2 children. All other operations have 0 or 1 
    children. Each node in the tree is an operation and the links to children
    are the pipeline.
 
  SEE ALSO
    query_tree.cc
*/
#include "query_tree.h"
#include "sql_base.h"
#include "sql_acl.h"
#include "sql_parse.h"
#include "lock.h"

/*
  Build Query Tree

  SYNOPSIS
    build_query_tree()
    THD *thd            IN the current thread
    LEX *lex            IN the pointer to the current parsed structure
    TABLE_LIST *tables  IN the list of tables identified in the query

  DESCRIPTION
    This method returns a converted MySQL internal representation (IR) of a
    query as a query_tree.

  RETURN VALUE
    Success = Query_tree * -- the root of the new query tree.
    Failed = NULL
*/
Query_tree *build_query_tree(THD *thd, LEX *lex, TABLE_LIST *tables)
{
  DBUG_ENTER("build_query_tree");
  Query_tree *qt = new Query_tree();
  Query_tree::query_node *qn = 
    (Query_tree::query_node *)my_malloc(sizeof(Query_tree::query_node), 
    MYF(MY_ZEROFILL | MY_WME));
  TABLE_LIST *table;
  int i = 0;
  Item *w;
  int num_tables = 0;

  /* create a new restrict node */
  qn->parent_nodeid = -1;
  qn->child = false;
  qn->join_type = (Query_tree::type_join) 0;
  qn->nodeid = 0;
  qn->node_type = (Query_tree::query_node_type) 2; 
  qn->left = NULL;
  qn->right = NULL;
  qn->attributes = new Attribute();
  qn->where_expr = new Expression();
  qn->join_expr = new Expression();

  if(lex->select_lex.options & SELECT_DISTINCT)
  {
    //qt->set_distinct(true); /* placeholder for exercise */
  }

  /* Get the tables (relations) */
  i = 0;
  for(table = tables; table; table = table->next_local)
  {
    num_tables++;
    qn->relations[i] = table;
    i++;
  }

  /* prepare the fields (find associated tables) for query */
  List <Item> all_fields;
  Name_resolution_context context;
  List_iterator <Item> it(thd->lex->select_lex.item_list);
  it++;
  if (lex->select_lex.with_wild)
  {
    bool found = FALSE;
    Field_iterator_table_ref field_iterator;
    for(table = tables; table; table = table->next_local)
    {
      field_iterator.set(table);
      for (; !field_iterator.end_of_fields(); field_iterator.next())
      {
        Item *item= field_iterator.create_item(thd);
        if (!found)
        {
          found= TRUE;
          it.replace(item); /* Replace '*' with the first found item. */
        }
        else
        {
          it.after(item);   /* Add 'item' to the SELECT list. */
        }
      }
    }
  }
  if (setup_fields(thd, lex->select_lex.ref_pointer_array,
                   lex->select_lex.item_list, thd->mark_used_columns,
                   &all_fields, 1))
    DBUG_RETURN(NULL);  
  qt->result_fields = lex->select_lex.item_list;
  
  /* get the attributes from the raw query */
  w = lex->select_lex.item_list.pop();
  while (w != 0)
  {
    uint unused_field_idx= NO_CACHED_FIELD_INDEX;
    TABLE_LIST *dummy;
    Field *f = NULL;
    for(table = tables; table; table = table->next_local)
    {
      f = find_field_in_table_ref(thd, table, ((Field *)w)->field_name,
                                  strlen(((Field *)w)->field_name),
                                  ((Field *)w)->field_name, NULL, NULL, NULL,
                                  FALSE, FALSE, &unused_field_idx, FALSE,
                                  &dummy);
      if (f)
      {
        qn->attributes->add_attribute(true, (Item *)f);
        break;
      }
    }
    w = lex->select_lex.item_list.pop();
  }

  /* get the joins from the raw query */
  if (num_tables > 0)  //indicates more than 1 table processed
    for(table = tables; table; table = table->next_local)
    {
      if (table->join_cond() != 0)
        qn->join_expr->convert(thd, (Item *)table->join_cond());
    }

  /* get the expressions for the where clause */
  qn->where_expr->convert(thd, lex->select_lex.where);

  /* get the join conditions for the joins */
  qn->join_expr->get_join_expr(qn->where_expr);

  /* if there is a where clause, set node to restrict */
  if (qn->where_expr->num_expressions() > 0)
    qn->node_type = (Query_tree::query_node_type) 1;

  qt->root = qn;
  DBUG_RETURN(qt);
}

int DBXP_explain_select_command(THD *thd);

/*
  Send data

  SYNOPSIS
    send_data()
    Protocol *p        IN the Protocol class 
    THD *thd           IN the current thread
    List<Item> *items  IN the list of fields identified in the row

  DESCRIPTION
    This method sends the data to the clien using the protocol class.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
bool send_data(Protocol *protocol, List<Item> &items, THD *thd)
{

  /* use a list iterator to loop through items */
  List_iterator_fast<Item> li(items);

  char buff[MAX_FIELD_WIDTH];
  String buffer(buff, sizeof(buff), &my_charset_bin);
  DBUG_ENTER("send_data");

  /* this call resets the transmission buffers */
  protocol->prepare_for_resend();

  /* for each item in the list (a field), send data to the client */
  Item *item;
  while ((item=li++))
  {
    /* 
      Use the MySQL send method for the item class to write to network.
      If unsuccessful, free memory and send error message to client.
    */
    if (item->send(protocol, &buffer))
    {
      protocol->free();        /* Free used buffer */
      my_message(ER_OUT_OF_RESOURCES, ER(ER_OUT_OF_RESOURCES), MYF(0));
      break;
    }
  }
  /* increment row count */
  thd->inc_sent_row_count(1);
 
  /* if network write was ok, return */
  if (thd->vio_ok())
    DBUG_RETURN(protocol->write());

  DBUG_RETURN(0);
}

/*
  Perform Select Command

  SYNOPSIS
    DBXP_select_command()
    THD *thd            IN the current thread

  DESCRIPTION
    This method executes the SELECT command using the query tree and optimizer.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int DBXP_select_command(THD *thd)
{
  DBUG_ENTER("DBXP_select_command");
  DBXP_explain_select_command(thd);
  DBUG_RETURN(0);
}

/*
  Write to vio with printf.

  SYNOPSIS
    write_printf()
    Protocol *p     IN the Protocol class 
    char *first     IN the first string to write
    char *last      IN the last string to write

  DESCRIPTION
    This method writes to the vio routines printing the strings passed.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int write_printf(Protocol *p, char *first, const char *last)
{
  char *str = (char *)my_malloc(256, MYF(MY_ZEROFILL | MY_WME));

  DBUG_ENTER("write_printf");
  strcpy(str, first);
  strcat(str, last);
  p->prepare_for_resend();
  p->store(str, system_charset_info);
  p->write();
  my_free(str);
  DBUG_RETURN(0);
}

/*
  Show Query Plan

  SYNOPSIS
    show_plan()
    Protocol *p         IN the MySQL protocol class
    query_node *Root    IN the root node of the query tree
    query_node *qn      IN the starting node to be operated on.
    bool print_on_right IN indicates the printing should tab to the right 
                           of the display.

  DESCRIPTION
    This method prints the execute plan to the client via the protocol class

  WARNING
    This is a RECURSIVE method!
    Uses postorder traversal to draw the quey plan

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int show_plan(Protocol *p, Query_tree::query_node *root, 
              Query_tree::query_node *qn, bool print_on_right)
{
  DBUG_ENTER("show_plan");

  /* spacer is used to fill white space in the output */
  char *spacer = (char *)my_malloc(80, MYF(MY_ZEROFILL | MY_WME));
  char *tblname = (char *)my_malloc(256, MYF(MY_ZEROFILL | MY_WME));

  if(qn != 0)
  {
    show_plan(p, root, qn->left, print_on_right);
    show_plan(p, root, qn->right, true);
    
    /* draw incoming arrows */
    if(print_on_right)
      strcpy(spacer, "          |               ");
    else
      strcpy(spacer, "     ");

    /* Write out the name of the database and table */
    if((qn->left == NULL) && (qn->right == NULL))
    {
      /*
         If this is a join, it has 2 children so we need to write
         the children nodes feeding the join node. Spaces are used
         to place the tables side-by-side.
      */
      if(qn->node_type == Query_tree::qntJoin)
      {
        strcpy(tblname, spacer);
        strcat(tblname, qn->relations[0]->db);
        strcat(tblname, ".");
        strcat(tblname, qn->relations[0]->table_name);
        if(strlen(tblname) < 15)
          strcat(tblname, "               ");
        else
          strcat(tblname, "          ");
        strcat(tblname, qn->relations[1]->db);
        strcat(tblname, ".");
        strcat(tblname, qn->relations[1]->table_name);
        write_printf(p, tblname, "");
        write_printf(p, spacer, "     |                              |");
        write_printf(p, spacer, "     |   ----------------------------");
        write_printf(p, spacer, "     |   |");
        write_printf(p, spacer, "     V   V");
      }
      else
      {
        strcpy(tblname, spacer);
        strcat(tblname, qn->relations[0]->db);
        strcat(tblname, ".");
        strcat(tblname, qn->relations[0]->table_name);
        write_printf(p, tblname, "");
        write_printf(p, spacer, "     |");
        write_printf(p, spacer, "     |");
        write_printf(p, spacer, "     |");
        write_printf(p, spacer, "     V");
      }
    }
    else if((qn->left != 0) && (qn->right != 0))
    {
      write_printf(p, spacer, "     |                              |");
      write_printf(p, spacer, "     |   ----------------------------");
      write_printf(p, spacer, "     |   |");
      write_printf(p, spacer, "     V   V");
    }
    else if((qn->left != 0) && (qn->right == 0))
    {
      write_printf(p, spacer, "     |");
      write_printf(p, spacer, "     |");
      write_printf(p, spacer, "     |");
      write_printf(p, spacer, "     V");
    }
    else if(qn->right != 0)
    {
    }
    write_printf(p, spacer, "-------------------");

    /* Write out the node type */
    switch(qn->node_type)
    {
    case Query_tree::qntProject:
      {
        write_printf(p, spacer, "|     PROJECT     |");
        write_printf(p, spacer, "-------------------");
        break;
      }
    case Query_tree::qntRestrict:
      {
        write_printf(p, spacer, "|    RESTRICT     |");
        write_printf(p, spacer, "-------------------");
        break;
      }
    case Query_tree::qntJoin:
      {
        write_printf(p, spacer, "|      JOIN       |");
        write_printf(p, spacer, "-------------------");
        break;
      }
    case Query_tree::qntDistinct:
      {
        write_printf(p, spacer, "|     DISTINCT    |");
        write_printf(p, spacer, "-------------------");
        break;
      }
    default:
      {
        write_printf(p, spacer, "|      UNDEF      |");
        write_printf(p, spacer, "-------------------");
        break;
      }
    }
    write_printf(p, spacer, "| Access Method:  |");
    write_printf(p, spacer, "|    iterator     |");
    write_printf(p, spacer, "-------------------");
    if(qn == root)
    {
      write_printf(p, spacer, "        |");
      write_printf(p, spacer, "        |");
      write_printf(p, spacer, "        V");
      write_printf(p, spacer, "    Result Set");
    }
  }
  my_free(spacer);
  my_free(tblname);
  DBUG_RETURN(0);
}


/*
  Perform EXPLAIN command.

  SYNOPSIS
    DBXP_explain_select_command()
    THD *thd            IN the current thread

  DESCRIPTION
    This method executes the EXPLAIN SELECT command.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int DBXP_explain_select_command(THD *thd)
{
  bool res;

  DBUG_ENTER("DBXP_explain_select_command");

  /* Prepare the tables (check access, locks) */
  res = check_table_access(thd, SELECT_ACL, thd->lex->query_tables, 0, 1, 1);
  if (res)
    DBUG_RETURN(1);
  res = open_and_lock_tables(thd, thd->lex->query_tables, 0,
                             MYSQL_LOCK_IGNORE_TIMEOUT);
  if (res)
    DBUG_RETURN(1);

  /* Create the query tree and optimize it */
  Query_tree *qt = build_query_tree(thd, thd->lex, 
           (TABLE_LIST*) thd->lex->select_lex.table_list.first);
  qt->heuristic_optimization();
  qt->cost_optimization();

  /* create a field list for returning the query plan */
  List<Item> field_list;

  /* use the protocol class to communicate to client */
  Protocol *protocol= thd->protocol;

  /* write the field to the client */
  field_list.push_back(new Item_empty_string("Execution Path",NAME_LEN));
  if (protocol->send_result_set_metadata(&field_list,
                            Protocol::SEND_NUM_ROWS | Protocol::SEND_EOF))
    DBUG_RETURN(TRUE);
  protocol->prepare_for_resend();
  
  /* generate the query plan and send it to client */
  show_plan(protocol, qt->root, qt->root, false);
  my_eof(thd); /* end of file tells client no more data is coming */

  /* unlock tables and cleanup memory */
  mysql_unlock_read_tables(thd, thd->lock);
  delete qt;
  DBUG_RETURN(0);
}
