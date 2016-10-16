/*
  query_tree.cc

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
    query_tree.h
*/
#include "query_tree.h"

Query_tree::query_node::query_node()
{
  where_expr = NULL;
  join_expr = NULL;
  child = false;
  join_cond = Query_tree::jcUN;
  join_type = Query_tree::jnUNKNOWN;
  left = NULL;
  right = NULL;
  nodeid = -1;
  node_type = Query_tree::qntUndefined;
  sub_query = false;
  parent_nodeid = -1;
}


Query_tree::query_node::~query_node()
{
  if(left)
    delete left;
  if(right)
    delete right;
}

Query_tree::~Query_tree(void)
{
  if(root)
    delete root;
}

