/*
  Query_tree.h

  DESCRIPTION
    This file contains the Query_tree class. It is responsible for containing the
    internal representation of the query to be executed. It provides methods for
    optimizing and forming and inspecting the query tree. This class is the very
    heart of the DBXP query capability! It also provides the ability to store
    a binary "compiled" form of the query.

  NOTES
    The data structure is a binary tree that can have 0, 1, or 2 children. Only
    Join operations can have 2 children. All other operations have 0 or 1 
    children. Each node in the tree is an operation and the links to children
    are the pipeline.
 
  SEE ALSO
    query_tree.cc
*/
#include "attribute.h"
#include "expression.h"
#include "sql_priv.h"
#include "sql_class.h"
#include "table.h"
#include "records.h"

const int MAXNODETABLES = 4;
const int LEFTCHILD = 0;
const int RIGHTCHILD = 1;

class Query_tree
{
public:
  enum query_node_type          //this enumeration lists the available
  {                              //query node (operations)
    qntUndefined = 0,
    qntRestrict = 1,
    qntProject = 2,
    qntJoin = 3,
    qntSort = 4,
    qntDistinct = 5
  };

  enum join_con_type            //this enumeration lists the available
  {                              //join operations supported
    jcUN = 0,
    jcNA = 1,
    jcON = 2,
    jcUS = 3
  };

  enum type_join                //this enumeration lists the available
  {                              //join types supported.
    jnUNKNOWN      = 0,          //undefined
    jnINNER        = 1,
    jnLEFTOUTER    = 2,
    jnRIGHTOUTER   = 3,
    jnFULLOUTER    = 4,
    jnCROSSPRODUCT = 5,
    jnUNION        = 6,
    jnINTERSECT    = 7
  };

    enum AggregateType          //used to add aggregate functions
    {
        atNONE      = 0,
        atCOUNT     = 1
    };

  /*
    STRUCTURE query_node

    DESCRIPTION
      This this structure contains all of the data for a query node:

      NodeId -- the internal id number for a node
      ParentNodeId -- the internal id for the parent node (used for insert)
      SubQuery -- is this the start of a subquery?
      Child -- is this a Left or Right child of the parent?
      NodeType -- synonymous with operation type
      JoinType -- if a join, this is the join operation
      join_con_type -- if this is a join, this is the "on" condition
      Expressions -- the expressions from the "where" clause for this node
      Join Expressions -- the join expressions from the "join" clause(s) 
      Relations[] -- the relations for this operation (at most 2)
      PreemptPipeline -- does the pipeline need to be halted for a sort?
      Fields -- the attributes for the result set of this operation
      Left -- a pointer to the left child node
      Right -- a pointer to the right child node
*/
  struct query_node            
  {
    query_node();
    ~query_node();
    int                 nodeid;
    int                 parent_nodeid;
    bool                sub_query;
    int                 child; 
    query_node_type     node_type;
    type_join           join_type;
    join_con_type       join_cond;
    Expression          *where_expr;
    Expression          *join_expr;
    TABLE_LIST          *relations[MAXNODETABLES];
    int                 eof[MAXNODETABLES];
    int                 ndx[MAXNODETABLES];
    bool                preempt_pipeline;
    Attribute           *attributes;
    query_node          *left;
    query_node          *right;
  };

  struct record_buff
  {
    uchar *field_ptr;
    long field_length;
    record_buff *next;
    record_buff *prev;
    READ_RECORD *record;
  };

  query_node *root;              //The ROOT node of the tree

  Query_tree(void);
  ~Query_tree(void);
  int init_node(query_node *qn);
  int heuristic_optimization();
  int cost_optimization();
  int insert_attribute(query_node *qn, Item *c);
  bool distinct;
  int prepare(query_node *qn);
  int cleanup(query_node *qn);
  bool Eof(query_node *qn);
  READ_RECORD *get_next(query_node *qn);
  List <Item> result_fields;

private:
  bool h_opt;              //has query been optimized (rules)?
  bool c_opt;              //has query been optimized (cost)?
  READ_RECORD *lbuff;
  READ_RECORD *rbuff;
  record_buff *left_record_buff;
  record_buff *right_record_buff;
  record_buff *left_record_buffer_ptr;
  record_buff *right_record_buffer_ptr;

  int push_projections(query_node *qn, query_node *pNode);
  query_node *find_projection(query_node *qn);
  bool is_leaf(query_node *qn);
  bool has_relation(query_node *qn, char *Table);
  bool has_attribute(query_node *qn, Item *a);
  int del_attribute(query_node *qn, Item *a);
  int push_restrictions(query_node *qn, query_node *pNode);
  query_node *find_restriction(query_node *qn);
  query_node *find_join(query_node *qn);
  int push_joins(query_node *qn, query_node *pNode);
  int prune_tree(query_node *prev, query_node *cur_node);
  int balance_joins(query_node *qn);
  int split_restrict_with_project(query_node *qn);
  int split_restrict_with_join(query_node *qn);
  int split_project_with_join(query_node *qn);
  bool find_table_in_tree(query_node *qn, char *tbl);
  bool find_table_in_expr(Expression *expr, char *tbl);
  bool find_attr_in_expr(Expression *expr, char *tbl, char *value);
  int apply_indexes(query_node *qn);
  bool do_restrict(query_node *qn, READ_RECORD *t);
  READ_RECORD *do_project(query_node *qn, READ_RECORD *t);
  READ_RECORD *do_join(query_node *qn);
  int find_index_in_expr(Expression *e, char *tbl);
  TABLE *get_table(query_node *qn);
  int insertion_sort(bool left, Field *field, READ_RECORD *rcd);
  int check_rewind(record_buff *cur_left, record_buff *curr_left_prev,
    record_buff *cur_right, record_buff *cur_right_prev);
};
