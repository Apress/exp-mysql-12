/*
  sql_dbxp_parse.cc

  DESCRIPTION
    This file contains methods to execute the DBXP SELECT query statements.
 
  SEE ALSO
    query_tree.cc
*/ 
#include "query_tree.h"

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
  Query_tree::query_node *qn = new Query_tree::query_node();
  TABLE_LIST *table;
  int i = 0;
  int num_tables = 0;

  /* Create a new restrict node. */
  qn->parent_nodeid = -1;
  qn->child = false;
  qn->join_type = (Query_tree::type_join) 0;
  qn->nodeid = 0;
  qn->node_type = (Query_tree::query_node_type) 2; 
  qn->left = 0;
  qn->right = 0;

  if(lex->select_lex.options & SELECT_DISTINCT)
  {
    //qt->set_distinct(true);
  }

  /* Get the tables (relations) */
  i = 0;
  for(table = tables; table; table = table->next_local)
  {
    num_tables++;
    qn->relations[i] = table;
    i++;
  }

  /* Populate attributes */
  qn->fields = &lex->select_lex.item_list;
  /* Process joins */
  if (num_tables > 0)  //indicates more than 1 table processed
    for(table = tables; table; table = table->next_local)
      if (((Item *)table->join_cond() != 0) && (qn->join_expr == 0))
        qn->join_expr = (Item *)table->join_cond();
  qn->where_expr = lex->select_lex.where;
  qt->root = qn;
  DBUG_RETURN(qt);
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
  Query_tree *qt = build_query_tree(thd, thd->lex, 
                                    (TABLE_LIST*) thd->lex->select_lex.table_list.first);
  List<Item> field_list;
  Protocol *protocol= thd->protocol;
  field_list.push_back(new Item_empty_string("Database Experiment Project (DBXP)",40));
  if (protocol->send_result_set_metadata(&field_list,
                            Protocol::SEND_NUM_ROWS | Protocol::SEND_EOF))
    DBUG_RETURN(TRUE);
  protocol->prepare_for_resend();
  protocol->store("Query tree was built.", system_charset_info);
  if (protocol->write())
    DBUG_RETURN(TRUE);
  my_eof(thd);
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
  char *str = new char[1024];

  DBUG_ENTER("write_printf");
  strcpy(str, first);
  strcat(str, last);
  p->prepare_for_resend();
  p->store(str, system_charset_info);
  p->write();
  delete str;
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
  int i = 0;

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
  DBUG_ENTER("DBXP_explain_select_command");
  Query_tree *qt = build_query_tree(thd, thd->lex, 
                                    (TABLE_LIST*) thd->lex->select_lex.table_list.first);
  List<Item> field_list;
  Protocol *protocol= thd->protocol;
  field_list.push_back(new Item_empty_string("Execution Path",NAME_LEN));
  if (protocol->send_result_set_metadata(&field_list,
                            Protocol::SEND_NUM_ROWS | Protocol::SEND_EOF))
    DBUG_RETURN(TRUE);
  protocol->prepare_for_resend();
  show_plan(protocol, qt->root, qt->root, false);
  my_eof(thd);
  DBUG_RETURN(0);
}
