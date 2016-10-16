/*
  query_tree.h

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
#include "sql_priv.h"
#include "sql_class.h"
#include "table.h"
#include "records.h"

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
    //query_node(const query_node &o);
    ~query_node();
    int                 nodeid;
    int                 parent_nodeid;
    bool                sub_query;
    bool                child;
    query_node_type     node_type;
    type_join           join_type;
    join_con_type       join_cond;
    Item                *where_expr;
    Item                *join_expr;
    TABLE_LIST          *relations[4];
    bool                preempt_pipeline;
    List<Item>          *fields;
    query_node          *left;
    query_node          *right;
  };

  query_node *root;              //The ROOT node of the tree

  ~Query_tree(void);
  void ShowPlan(query_node *QN, bool PrintOnRight);

};
