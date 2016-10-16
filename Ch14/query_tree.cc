/*
  Query_tree.cc

  DESCRIPTION
    This file contains the Query_tree class. It is responsible for containing the
    internal representation of the query to be executed. It provides methods for
    optimizing and forming and inspecting the query tree. This class is the very
    heart of the DBXP query capability! It also provides the ability to store
    a binary "compiled" form of the query.

  NOTES
    The data structure is a binary tree that can have 0, 1, or 2 children. Only
    join operations can have 2 children. All other operations have 0 or 1 
    children. Each node in the tree is an operation and the links to children
    are the pipeline.
 
  SEE ALSO
    query_tree.h
*/
#include "query_tree.h"

/*
  Constructor

  SYNOPSIS
    Query_tree::query_node()

  DESCRIPTION
    Initializes the query_node properties and arrays.
*/
Query_tree::query_node::query_node()
{
  int i; 

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
  for (i = 0; i < MAXNODETABLES; i++)
  {
    relations[i] = NULL;
    eof[i] = 0;
    ndx[i] = -1;
  }
  parent_nodeid = -1;
}

/*
  Initialize query node

  SYNOPSIS
    init_node()

  DESCRIPTION
    Initializes the query_node properties and arrays.
*/
int Query_tree::init_node(query_node *qn)
{
  int i; 

  DBUG_ENTER("init_node");
  qn->where_expr = NULL;
  qn->join_expr = NULL;
  qn->child = false;
  qn->join_cond = Query_tree::jcUN;
  qn->join_type = Query_tree::jnUNKNOWN;
  qn->left = NULL;
  qn->right = NULL;
  qn->nodeid = -1;
  qn->node_type = Query_tree::qntUndefined;
  qn->sub_query = false;
  for (i = 0; i < MAXNODETABLES; i++)
  {
    qn->relations[i] = NULL;
    qn->eof[i] = 0;
    qn->ndx[i] = -1;
  }
  qn->parent_nodeid = -1;
  DBUG_RETURN(0);
}

/*
  Destructor

  SYNOPSIS
    Query_tree::query_node()

  DESCRIPTION
    Destroys dynamic memory in the node.
*/
Query_tree::query_node::~query_node()
{
  if(left)
    my_free(left);
  if(right)
    my_free(right);
}

/*
  Constructor

  SYNOPSIS
    Query_tree()

  DESCRIPTION
    Initializes the query_tree properties and arrays.
*/
Query_tree::Query_tree(void)
{
  distinct = false;
}

/*
  Destructor

  SYNOPSIS
    Query_tree()

  DESCRIPTION
    Destroys dynamic memory in the tree.
*/
Query_tree::~Query_tree(void)
{
  if(root)
    my_free(root);
}

/*
  Perform heuristic optimization

  SYNOPSIS
    heuristic_optimization()

  DESCRIPTION
    This method performs heuristic optimization on the query tree. The 
    operation is destructive in that it rearranges the original tree.

  RETURN VALUE
    Success = 0  
    Failed = 1
*/
int Query_tree::heuristic_optimization()
{
  DBUG_ENTER("heuristic_optimization");
  query_node       *pNode;
  query_node       *nNode;

  h_opt = true;
  /*
    First, we have to correct the situation where restrict and
    project are grouped together in the same node.
  */
  split_restrict_with_join(root);
  split_project_with_join(root);
  split_restrict_with_project(root);

  /*
    Find a node with restrictions and push down the tree using 
    a recursive call. continue until you get the same node twice.
    This means that the node cannot be pushed down any further.
  */
  pNode = find_restriction(root);
  while(pNode != 0)
  {
    push_restrictions(root, pNode);
    nNode = find_restriction(root);
    /*
      If a node is found, save a reference to it unless it is
      either the same node as the last node found or
      it is a leaf node. This is done so that we can ensure we
      continue searching down the tree visiting each node once.
    */
    if(nNode != 0)
    {
      if(nNode->nodeid == pNode->nodeid)
        pNode = 0;
      else if(is_leaf(nNode))
        pNode = 0;
      else
        pNode = nNode;
    }
  }

  /*
    Find a node with projections and push down the tree using 
    a recursive call. Continue until you get the same node twice.
    This means that the node cannot be pushed down any further.
  */
  pNode = find_projection(root);
  while(pNode != 0)
  {
    push_projections(root, pNode);
    nNode = find_projection(root);
    /*
      If a node is found, save a reference to it unless it is
      either the same node as the last node found or
      it is a leaf node. This is done so that we can ensure we
      continue searching down the tree visiting each node once.
    */
    if(nNode != 0)
    {
      if(nNode->nodeid == pNode->nodeid)
        pNode = 0;
      else if(is_leaf(nNode))
        pNode = 0;
      else
        pNode = nNode;
    }
  }

  /*
    Find a join node and push it down the tree using 
    a recursive call. Continue until you get the same node twice.
    This means that the node cannot be pushed down any further.
  */
  pNode = find_join(root);
  while(pNode != 0)
  {
    push_joins(root, pNode);
    nNode = find_join(root);
    /*
      If a node is found, save a reference to it unless it is
      either the same node as the last node found or
      it is a leaf node. This is done so that we can ensure we
      continue searching down the tree visiting each node once.
    */
    if(nNode != 0)
    {
      if(nNode->nodeid == pNode->nodeid)
        pNode = 0;
      else if(is_leaf(nNode))
        pNode = 0;
      else
        pNode = nNode;
    }
    else
      pNode = nNode;
  }

  /*
    Prune the tree of "blank" nodes
    Blank Nodes are:
     1) projections without attributes that have at least 1 child
     2) restrictions without expressions
     BUT...Can't delete a node that has TWO children!
  */
  prune_tree(0, root);

  /*
    Lastly, check to see if this has the DISTINCT option.
    If so, create a new node that is a DISTINCT operation.
  */
  if(distinct && (root->node_type != qntDistinct))
  {
    int i;
    pNode = (query_node*)my_malloc(sizeof(query_node), 
            MYF(MY_ZEROFILL | MY_WME));
    init_node(pNode);
    pNode->sub_query = 0;
    pNode->attributes = 0;
    pNode->join_cond = jcUN;  /* (join_con_type) 0; */
    pNode->join_type = jnUNKNOWN;  /* (type_join) 0; */
    pNode->left = root;
    pNode->right = 0;
    for(i = 0; i < MAXNODETABLES; i++)
      pNode->relations[i] = NULL;
    pNode->nodeid = 90125;
    pNode->child = LEFTCHILD;
    root->parent_nodeid = 90125;
    root->child = LEFTCHILD;
    pNode->parent_nodeid = -1;
    pNode->node_type = qntDistinct;
    pNode->attributes = new Attribute();
    pNode->where_expr = new Expression();
    pNode->join_expr = new Expression();
    root = pNode;
  }
  DBUG_RETURN(0);
}

/*
  Prune the tree of dead limbs.

  SYNOPSIS
    prune_tree()
    query_node *prev IN the previous node (parent)
    query_node *cur_node IN the current node pointer (used to delete).

  DESCRIPTION
    This method looks for nodes blank nodes that are a result of performing
    heuristic optimization on the tree and deletes them.

  NOTES
    This is a RECURSIVE method!

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::prune_tree(query_node *prev, query_node *cur_node)
{
  DBUG_ENTER("prune_tree");
  if(cur_node != 0)
  {
    /* 
      Blank Nodes are 1) projections without attributes 
      that have at least 1 child, or 2) restrictions 
      without expressions
    */
    if((((cur_node->node_type == qntProject) && 
       (cur_node->attributes->num_attributes() == 0)) ||
      ((cur_node->node_type == qntRestrict) && 
       (cur_node->where_expr->num_expressions() == 0))) &&
       ((cur_node->left == 0) || (cur_node->right == 0)))
    {
      /* 
        Redirect the pointers for the nodes above and
        below this node in the tree.
      */
      if(prev == 0) 
      {
         if(cur_node->left == 0)
        {
          cur_node->right->parent_nodeid = -1;
          root = cur_node->right;
        }
        else
        {
          cur_node->left->parent_nodeid = -1;
          root = cur_node->left;
        }
        my_free(cur_node);
        cur_node = root;
      }
      else
      {
        if(prev->left == cur_node)
        {
          if(cur_node->left == 0)
          {
            prev->left = cur_node->right;
            if (cur_node->right != NULL)
              cur_node->right->parent_nodeid = prev->nodeid;
          }
          else
          {
            prev->left = cur_node->left;
            if (cur_node->left != NULL)
              cur_node->left->parent_nodeid = prev->nodeid;
          }
          my_free(cur_node);
          cur_node = prev->left;
        }
        else
        {
          if(cur_node->left == 0)
          {
            prev->right = cur_node->right;
            if (cur_node->right != NULL)
              cur_node->right->parent_nodeid = prev->nodeid;
          }
          else
          {
            prev->right = cur_node->left;
            if (cur_node->left != NULL)
              cur_node->left->parent_nodeid = prev->nodeid;
          }
          my_free(cur_node);
          cur_node = prev->right;
        }
      }
      prune_tree(prev, cur_node);
    }
    else
    {
      prune_tree(cur_node, cur_node->left);
      prune_tree(cur_node, cur_node->right);
    }
  }
  DBUG_RETURN(0);
}


/*
  Find a projection in the tree

  SYNOPSIS
    find_projection()
    query_node *QN IN the node to operate on

  DESCRIPTION
    This method looks for a node containing a projection and returns the node
    pointer.

  NOTES
    This finds the first projection and is biased to the left tree.
    This is a RECURSIVE method!

  RETURN VALUE
    Success = query_node * the node located or NULL for not found
    Failed = NULL
*/
Query_tree::query_node *Query_tree::find_projection(query_node *QN)
{
  DBUG_ENTER("find_projection");
  query_node   *N;

  N = 0;
  if(QN != 0)
  {
    /* 
      A projection is a node marked as project and 
      has at least one attribute
    */
    if((QN->node_type == qntProject) &&
       (QN->attributes != 0))
      N = QN;
    else
    {
      N = find_projection(QN->left);
      if(N == 0)
        N = find_projection(QN->right);
    }
  }
  DBUG_RETURN(N);
}

/*
  Push projections down the tree.

  SYNOPSIS
    push_projections()
    query_node *QN IN the node to operate on
    query_node *pNode IN the node containing the projection attributes

  DESCRIPTION
    This method looks for projections and pushes them down the tree to nodes
    that contain the relations specified.

  NOTES
    This is a RECURSIVE method!

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::push_projections(query_node *QN, query_node *pNode)
{
  DBUG_ENTER("push_projections");
  Item *   a;
  int         i;
  int         j;

  if((QN != 0) && (pNode != 0))
  {
    if((QN->nodeid != pNode->nodeid) &&
       (QN->node_type == qntProject))
    {
      i = 0;
      j = QN->attributes->num_attributes();
      
      /* move attributes to new node */
      while(i < j)
      {
        a = QN->attributes->get_attribute(i);
        if(has_relation(QN, 
          (char *)((Field *)a)->table->s->table_name.str))
        {
          if(!has_attribute(QN, a))
            insert_attribute(QN, a);
          del_attribute(pNode, a);
        }
        i++;
      }
    }
    if(pNode->attributes->num_attributes() != 0)
    {
      push_projections(QN->left, pNode);
      push_projections(QN->right, pNode);
    }
  }
  DBUG_RETURN(0);
}

/*
  Delete an attribute from a node.

  SYNOPSIS
    del_attribute()
    query_node *QN IN the node to operate on
    Item *a IN the attribute to remove

  DESCRIPTION
    This method removes the attribute specified.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::del_attribute(query_node *QN, Item *a)
{
  DBUG_ENTER("del_attribute(query_node *QN, Item * a)");
  if(QN != 0)
  {
    QN->attributes->remove_attribute(QN->attributes->index_of(
      (char *)((Field *)a)->table->s->table_name.str,
      (char *)((Field *)a)->field_name));
  }
  DBUG_RETURN(0);
}

/*
  Check to see if the node has the table.

  SYNOPSIS
    has_relation()
    query_node *QN IN the node to operate on
    char *Table IN the relation you're looking for

  DESCRIPTION
    This method returns TRUE if the node contains the relation specified.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::has_relation(query_node *QN, char *table)
{
  DBUG_ENTER("has_relation(query_node *QN, char *Table)");
  bool    found = false;
  int     i = 0;

  if(QN != 0)
  {
    while((QN->relations[i] != NULL) && (table != NULL))
    {
      if(strcmp(QN->relations[i]->table_name, table) == 0)
        found = true;
      i++;
    }
  }
  DBUG_RETURN(found);
}

/*
  Check to see if the node has the attribute.

  SYNOPSIS
    has_attribute()
    query_node *QN IN the node to operate on
    Item *a IN the attribute you're looking for

  DESCRIPTION
    This method returns TRUE if the node contains the attribute specified.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::has_attribute(query_node *QN, Item *a)
{
  DBUG_ENTER("has_attribute");
  bool        found = false;
  int         i = -1;

  if(QN != 0)
  {
    i = QN->attributes->index_of(
      (char *)((Field *)a)->table->s->table_name.str,
      (char *)((Field *)a)->field_name);
    if(i >= 0)
      found = true;
  }
  DBUG_RETURN(found);
}

/*
  Is the node a leaf?

  SYNOPSIS
    is_leaf()
    query_node *QN IN the node to operate on

  DESCRIPTION
    This method returns TRUE if the node specified is a leaf (no children).

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::is_leaf(query_node *QN)
{
  DBUG_ENTER("is_leaf");
  DBUG_RETURN((QN->left == 0) && (QN->right == 0));
}

/*
  Insert an attribute into the node.

  SYNOPSIS
    insert_attribute()
    query_node *QN IN the node to operate on
    Item *c IN the attribute to add

  DESCRIPTION
    This method is used to place an attribute (c) in the query node (QN).

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::insert_attribute(query_node *QN, Item *c)
{
  DBUG_ENTER("insert_attribute");
  if(QN != 0)
    QN->attributes->add_attribute(true, c);
  DBUG_RETURN(0);
}

/*
  Find a restriction in the subtree.
  
  SYNOPSIS
    find_restriction()
    query_node *QN IN the node to operate on

  DESCRIPTION
    This method looks for a node containing a restriction and returns the node
    pointer.

  NOTES
    This is a RECURSIVE method!
    This finds the first restriction and is biased to the left tree.

  RETURN VALUE
    Success = query_node * the node located
    Failed = NULL
*/
Query_tree::query_node *Query_tree::find_restriction(query_node *QN)
{
  DBUG_ENTER("find_restriction");
  query_node   *N;

  N = 0;
  if(QN != 0)
  {
    /* 
      A restriction is a node marked as restrict and 
      has at least one expression
    */
    if (QN->where_expr->num_expressions() > 0)
      N = QN;
    else
    {
      N = find_restriction(QN->left);
      if(N == 0)
        N = find_restriction(QN->right);
    }
  }
  DBUG_RETURN(N);
}

/*
  Push restrictions down the tree.

  SYNOPSIS
    push_restrictions()
    query_node *QN IN the node to operate on
    query_node *pNode IN the node containing the restriction attributes

  DESCRIPTION
    This method looks for restrictions and pushes them down the tree to nodes
    that contain the relations specified.

  NOTES
    This is a RECURSIVE method!
    This finds the first restriction and is biased to the left tree.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::push_restrictions(query_node *QN, query_node *pNode)
{
  query_node *NewQN=0;

  DBUG_ENTER("push_restrictions");
  if((QN != 0) && (pNode != 0) && (pNode->left != 0))
  {
    /*
      Conditions:
        1) QN is a join node
        2) QN is a project node
        3) QN is a restrict node 
        4) All other nodes types are ignored.
      
      Methods:
        1) if join or project and the children are not already restrictions
           add a new node and put where clause in new node else
           see if you can combine the child node and this one
        2) if the node has the table and it is a join,
           create a new node below it and push the restriction
           to that node.
        4) if the node is a restriction and has the table,
           just add the expression to the node's expression list
    */

    /* if projection, move node down tree */
    if((QN->nodeid != pNode->nodeid) && (QN->node_type == qntProject))
    {
      if (QN->left != 0)
      {
        QN->left = (query_node*)my_malloc(sizeof(query_node), 
                   MYF(MY_ZEROFILL | MY_WME));
        init_node(QN->left);
        NewQN = QN->left;
        NewQN->left = 0;
      }
      else
      {
        NewQN = QN->left;
        QN->left = (query_node*)my_malloc(sizeof(query_node), 
                   MYF(MY_ZEROFILL | MY_WME));
        QN->left->left = NewQN;
        NewQN = QN->left;
      }
      NewQN->sub_query = 0;
      NewQN->join_cond = jcUN;  /* (join_con_type) 0; */
      NewQN->join_type = jnUNKNOWN;  /* (type_join) 0; */
      NewQN->right = 0;
      for(long i = 0; i < MAXNODETABLES; i++)
        NewQN->relations[i] = 0;
      NewQN->nodeid = QN->nodeid + 1;
      NewQN->parent_nodeid = QN->nodeid;
      NewQN->node_type = qntRestrict;
      NewQN->attributes = new Attribute();
      NewQN->where_expr = new Expression();
      NewQN->join_expr = new Expression();
      if (pNode->relations[0])
        NewQN->where_expr->reduce_expressions(pNode->relations[0]->table);
      if ((QN->relations[0] != NULL) && (QN->relations[0] == pNode->relations[0]))
      {
        if (QN->relations[0])
          if (find_table_in_expr(pNode->where_expr, QN->relations[0]->table_name))
          {
            NewQN->relations[0] = QN->relations[0];
            QN->relations[0] = 0;
          }
      }
      else 
      {
        if (pNode->relations[0])
          if (find_table_in_tree(QN->left, pNode->relations[0]->table_name))
            NewQN->relations[0] = 0;
        pNode->where_expr = NULL;
        pNode->relations[0] = 0;
      }
    }
    /* if join, move restrict node down tree */
    else if((QN->nodeid != pNode->nodeid) && 
      ((QN->left == 0) || (QN->right == 0)) &&
      (QN->node_type == qntJoin))
    {
      if(QN->relations[0] != 0)
      {
        QN->left = (query_node*)my_malloc(sizeof(query_node), 
                   MYF(MY_ZEROFILL | MY_WME));
        NewQN = QN->left;
        NewQN->sub_query = 0;
        NewQN->join_cond = jcUN;  /* (join_con_type) 0; */
        NewQN->join_type = jnUNKNOWN;  /* (type_join) 0; */
        NewQN->left = 0;
        NewQN->right = 0;
        for(long i = 0; i < MAXNODETABLES; i++)
          NewQN->relations[i] = 0;
        NewQN->nodeid = QN->nodeid + 1;
        NewQN->parent_nodeid = QN->nodeid;
        NewQN->node_type = qntRestrict;
        NewQN->attributes = new Attribute();
        NewQN->where_expr = new Expression();
        NewQN->join_expr = new Expression();
        NewQN->relations[0] = QN->relations[0];
        QN->relations[0] = 0;
        if (pNode->relations[0])
          NewQN->where_expr->reduce_expressions(pNode->relations[0]->table);
      }
      else if(QN->relations[1] != 0)
      {
        QN->right = (query_node*)my_malloc(sizeof(query_node), 
                    MYF(MY_ZEROFILL | MY_WME));
        NewQN = QN->left;
        NewQN->sub_query = 0;
        NewQN->join_cond = jcUN;  /* (join_con_type) 0; */
        NewQN->join_type = jnUNKNOWN;  /* (type_join) 0; */
        NewQN->left = 0;
        NewQN->right = 0;
        for(long i = 0; i < MAXNODETABLES; i++)
          NewQN->relations[i] = 0;
      }
      NewQN->nodeid = QN->nodeid + 1;
      NewQN->parent_nodeid = QN->nodeid;
      NewQN->node_type = qntRestrict;
      NewQN->attributes = new Attribute();
      NewQN->where_expr = new Expression();
      NewQN->join_expr = new Expression();
      NewQN->relations[0] = QN->relations[1];
      QN->relations[1] = 0;
      NewQN->where_expr->reduce_expressions(pNode->relations[0]->table);
    }
    push_restrictions(QN->left, pNode);
    push_restrictions(QN->right, pNode);
  }
   DBUG_RETURN(0);
}

/*
  Find a join in the subtree.
  
  SYNOPSIS
    find_restriction()
    query_node *QN IN the node to operate on

  DESCRIPTION
    This method looks for a node containing a join and returns the 
    node pointer.

  NOTES
    This is a RECURSIVE method!
    This finds the first restriction and is biased to the left tree.

  RETURN VALUE
    Success = query_node * the node located
    Failed = NULL
*/
Query_tree::query_node *Query_tree::find_join(query_node *QN)
{
  DBUG_ENTER("find_join");
  query_node               *N;
  N = 0;

  if(QN != 0)
  {
    /* 
      if this is a restrict node or a restrict node with 
      at least one expression it could be an unprocessed join
      because the default node type is restrict
    */
    if(((QN->node_type == qntRestrict) ||
      (QN->node_type == qntRestrict)) && (QN->join_expr->num_expressions() > 0))
      N = QN;
    else
    {
      N = find_join(QN->left);
      if(N == 0)
        N = find_join(QN->right);
    }
  }
  DBUG_RETURN(N);
}

/*
  Push joins down the tree.

  SYNOPSIS
    push_restrictions()
    query_node *QN IN the node to operate on
    query_node *pNode IN the node containing the join

  DESCRIPTION
    This method looks for theta joins and pushes them down the tree to the 
    parent of two nodes that contain the relations specified.

  NOTES
    This is a RECURSIVE method!

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::push_joins(query_node *QN, query_node *pNode)
{
  DBUG_ENTER("push_joins");
  Item *lField;
  Item *rField;
  expr_node *node;
  
  if(!pNode->join_expr)
    DBUG_RETURN(0);
  node = pNode->join_expr->get_expression(0);
  if (!node)
    DBUG_RETURN(0);
  lField = node->left_op;
  rField = node->right_op;

  /* Node must have expressions and not be null */
  if((QN != NULL) && (pNode != NULL) && 
     (pNode->join_expr->num_expressions() > 0))
  {
    /* check to see if tables in join condition exist */
    if((QN->nodeid != pNode->nodeid) &&
       (QN->node_type == qntJoin) &&
        QN->join_expr->num_expressions() == 0 &&
       ((has_relation(QN->left, 
         (char *)((Field *)lField)->table->s->table_name.str) &&
       has_relation(QN->right, 
         (char *)((Field *)rField)->table->s->table_name.str)) ||
      (has_relation(QN->left, 
        (char *)((Field *)rField)->table->s->table_name.str) && 
       has_relation(QN->right, 
         (char *)((Field *)lField)->table->s->table_name.str))))
    {
      /* move the expression */
      QN->join_expr = pNode->join_expr;
      pNode->join_expr = new Expression();
      QN->join_type = jnINNER;
      QN->join_cond = jcON;
    }
    push_joins(QN->left, pNode);
    push_joins(QN->right, pNode);
  }
  DBUG_RETURN(0);
}

/*
  Balance multiway joins to place most restrictive joins at lower level.

  SYNOPSIS
    balance_joins()
    query_node *QN IN the node to operate on

  DESCRIPTION
    This method will balance the joins once cost-based factors are applied.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::balance_joins(query_node *QN)
{
  DBUG_ENTER("balance_joins");
  DBUG_RETURN(0);
}

/*
  Perform cost optimization on the query tree.

  SYNOPSIS
    cost_optimization()

  DESCRIPTION
    This method performs cost-based optimization on the query tree. The 
    operation is nondestructive.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::cost_optimization()
{
  DBUG_ENTER("cost_optimization");
  c_opt = true;

  balance_joins(root);
  apply_indexes(root);
  DBUG_RETURN(0);
}

/*
  Walk the tree and choose best indexes for table access.

  SYNOPSIS
    apply_indexes()
    query_node *QN IN the node to operate on

  DESCRIPTION
    Search the tree looking for all possible indexes that can be used 
    and apply them to the SelIndex variable(s);

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::apply_indexes(query_node *QN)
{
  DBUG_ENTER("apply_indexes");
  DBUG_RETURN(0);
}

/*
  Split restrictions that have attributes (projections).

  SYNOPSIS
    split_restrict_with_project()
    query_node *QN IN the node to operate on

  DESCRIPTION
    This method looks for restrictions that have attributes (thus are both
    projections and restrictions) and breaks them into two nodes.

  NOTES
    This is a RECURSIVE method!

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::split_restrict_with_project(query_node *QN)
{
  DBUG_ENTER("split_restrict_with_project");
  if(QN != 0)
  {
    if(((QN->attributes->num_attributes() > 0) &&
      (QN->where_expr->num_expressions() > 0)) &&
      ((QN->node_type == qntProject) || (QN->node_type == qntRestrict)))
    {
      /*
        Create a new node and:
          1) Move the expressions to the new node.
          2) Set the new node's children = current node children
          3) Set the new node's relations = current node relations.
          4) Set current node's left child = new node;
          5) Set new node's id = current id + 1000;
          6) set parent id, etc.
      */
      query_node *new_node = (query_node*)my_malloc(sizeof(query_node), 
                             MYF(MY_ZEROFILL | MY_WME));
      init_node(new_node);
      new_node->child = LEFTCHILD;
      new_node->node_type = qntRestrict;
      if(new_node->node_type == qntJoin)
      {
        new_node->join_cond = QN->join_cond;
        new_node->join_type = QN->join_type;
      }
      QN->node_type = qntProject;
      new_node->attributes = new Attribute();
      new_node->where_expr = QN->where_expr;
      new_node->join_expr = new Expression();
      QN->where_expr = new Expression();
      new_node->left = QN->left;
      new_node->right = QN->right;
      new_node->parent_nodeid = QN->nodeid;
      new_node->nodeid = QN->nodeid + 1000;
      if(new_node->left)
        new_node->left->parent_nodeid = new_node->nodeid;
      if(new_node->right)
        new_node->right->parent_nodeid = new_node->nodeid;
      for(int i = 0; i < MAXNODETABLES; i++)
      {
        new_node->relations[i] = QN->relations[i];
        QN->relations[i] = NULL;
      }
      QN->left = new_node;
      QN->right = 0;
    }
    split_restrict_with_project(QN->left);
    split_restrict_with_project(QN->right);
  }
  DBUG_RETURN(0);
}

/*  
  Split restrictions that have joins.

  SYNOPSIS
    split_restrict_with_join()
    query_node *QN IN the node to operate on

  DESCRIPTION
     This method looks for joins that have where expressions (thus are both
     joins and restrictions) and breaks them into two nodes.

  NOTES
    This is a RECURSIVE method!

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::split_restrict_with_join(query_node *QN)
{
  int j = 0;
  int i = 0;

  DBUG_ENTER("split_restrict_with_join");
  if(QN != 0)
  {
    if(((QN->join_expr->num_expressions() > 0) &&
      (QN->where_expr->num_expressions() > 0)) &&
       ((QN->node_type == qntJoin) || (QN->node_type == qntRestrict)))
    {
      bool isleft = true;
      /*
        Create a new node and:
          1) Move the where expressions to the new node.
          2) Set the new node's children = current node children
          3) Set the new node's relations = current node relations.
          4) Set current node's left or right child = new node;
          5) Set new node's id = current id + 200;
           6) set parent id, etc.
          7) determine which table needs to be used for the 
             restrict node.
      */
      query_node *new_node = (query_node*)my_malloc(sizeof(query_node), 
                             MYF(MY_ZEROFILL | MY_WME));
      init_node(new_node);
      new_node->node_type = qntRestrict;
      new_node->parent_nodeid = QN->nodeid;
      new_node->nodeid = QN->nodeid + 200;
      new_node->where_expr = QN->where_expr;
      new_node->join_expr = new Expression();
      QN->where_expr = new Expression();

      /* 
        Loop through tables and move table that matches
        to the new node
      */
      for(i = 0; i < MAXNODETABLES; i++)
      {
        if (QN->relations[i] != NULL)
        {
          if (find_table_in_expr(new_node->where_expr, 
                QN->relations[i]->table_name))
          {
            new_node->relations[j] = QN->relations[i];
            j++;
            if (i != 0)
              isleft = false;
            QN->relations[i] = NULL;
          }
        }
      }

      /* set children to point to balance of tree */
      new_node->right = 0;
      if (isleft)
      {
        new_node->child = LEFTCHILD;
        new_node->left = QN->left;
        QN->left = new_node;
      }
      else
      {
        new_node->child = RIGHTCHILD;
        new_node->left = QN->right;
        QN->right = new_node;
      }
      if (new_node->left)
        new_node->left->parent_nodeid = new_node->nodeid;
      j = QN->attributes->num_attributes(); 
      if ((QN->node_type == qntJoin) && (j > 0))
      {
        Attribute *attribs = 0;
        Item * attr;
        int ii = 0;
        int jj = 0;
        if ((QN->attributes->num_attributes() == 1) &&
            (strcasecmp("*", 
            ((Field *)QN->attributes->get_attribute(0))->field_name) == 0))
        {
          new_node->attributes = new Attribute();
          new_node->attributes->add_attribute(j, 
            QN->attributes->get_attribute(0));
        }
        else
        {
          attribs = new Attribute();
          for (i = 0; i < (int)new_node->relations[0]->table->s->fields; i++)
          {
            Item *f = (Item *)new_node->relations[0]->table->field[i];
            attribs->add_attribute(true, (Item *)f);
          }
          j = attribs->num_attributes();
          new_node->attributes = new Attribute();
          for (i = 0; i < j; i++)
          {
            attr = attribs->get_attribute(i);
            jj = QN->attributes->index_of(
              (char *)((Field *)attr)->table->s->table_name.str, 
              (char *)((Field *)attr)->field_name);
            if (jj > -1)
            {
              new_node->attributes->add_attribute(ii, attr);
              ii++;
              QN->attributes->remove_attribute(jj);
            }
            else if (find_attr_in_expr(QN->join_expr, 
             (char *)((Field *)attr)->table->s->table_name.str, 
             (char *)((Field *)attr)->field_name)) 
            {
              new_node->attributes->add_attribute(ii, attr);
              new_node->attributes->hide_attribute(attr, true);
              ii++;
            }
          }
        }
      }
      else
      {
        QN->node_type = qntJoin;
        QN->join_type = jnINNER;
        new_node->attributes = new Attribute();
      }
    }
    split_restrict_with_join(QN->left);
    split_restrict_with_join(QN->right); 
  }
  DBUG_RETURN(0);
}

/*
  Split projections that have joins.

  SYNOPSIS
    split_project_with_join()
    query_node *QN IN the node to operate on

  DESCRIPTION
     This method looks for joins that have attributes (thus are both
     joins and projections) and breaks them into two nodes.

  NOTES
    This is a RECURSIVE method!

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::split_project_with_join(query_node *QN)
{
  int j = 0;
  int i;

  DBUG_ENTER("split_project_with_join");
  if(QN != 0)
  {
    if((QN->join_expr->num_expressions() > 0) &&
       ((QN->node_type == qntJoin) || (QN->node_type == qntProject)))
    {
      /*
        Create a new node and:
          1) Move the where expressions to the new node.
          2) Set the new node's children = current node children
          3) Set the new node's relations = current node relations.
          4) Set current node's left or right child = new node;
          5) Set new node's id = current id + 300;
          6) set parent id, etc.
      */
      QN->node_type = qntJoin;
      QN->join_type = jnINNER;
      if (QN->left == 0)
      {
        query_node *new_node = (query_node*)my_malloc(sizeof(query_node), 
                               MYF(MY_ZEROFILL | MY_WME));
        init_node(new_node);
        new_node->node_type = qntProject;
        new_node->parent_nodeid = QN->nodeid;
        new_node->nodeid = QN->nodeid + 300;
        for(i = 0; i < MAXNODETABLES; i++)
          new_node->relations[i] = 0;
        new_node->relations[0] = QN->relations[0];
        QN->relations[0] = 0;
        new_node->left = QN->left;
        QN->left = new_node;
        new_node->right = 0;
        new_node->child = LEFTCHILD;
        if (new_node->left != 0)
           new_node->left->parent_nodeid = new_node->nodeid;
        j = QN->attributes->num_attributes();
        new_node->attributes = new Attribute();
        new_node->where_expr = new Expression();
        new_node->join_expr = new Expression();
        if ((j == 1) &&
            (strcasecmp("*", ((Field *)QN->attributes->get_attribute(0))->field_name)==0))
        {
          new_node->attributes = new Attribute();
          new_node->attributes->add_attribute(j, QN->attributes->get_attribute(0));
          if (QN->right != 0)
            QN->attributes->remove_attribute(0);
        }
        else if (j > 0)
        {
          Attribute *attribs = 0;
          Item * attr;
          int ii = 0;
          int jj = 0;
          attribs = new Attribute();
          for (i = 0; i < (int)new_node->relations[0]->table->s->fields; i++)
          {
            Field *f = new_node->relations[0]->table->field[i];
            attribs->add_attribute(true, (Item *)f);
          }
          j = attribs->num_attributes();
          for (i = 0; i < j; i++)
          {
            attr = attribs->get_attribute(i);
            jj = QN->attributes->index_of(
              (char *)((Field *)attr)->table->s->table_name.str, 
              (char *)((Field *)attr)->field_name);
            if (jj > -1)
            {
              new_node->attributes->add_attribute(ii, attr);
              ii++;
              QN->attributes->remove_attribute(jj);
            }
            else if (find_attr_in_expr(QN->join_expr, 
              (char *)((Field *)attr)->table->s->table_name.str, 
              (char *)((Field *)attr)->field_name)) 
            {
              new_node->attributes->add_attribute(ii, attr);
              new_node->attributes->hide_attribute(attr, true);
              ii++;
            }
          }
        }
      }
      if (QN->right == 0)
      {
        query_node *new_node = (query_node*)my_malloc(sizeof(query_node), 
                               MYF(MY_ZEROFILL | MY_WME));
        init_node(new_node);
        new_node->node_type = qntProject;
        new_node->parent_nodeid = QN->nodeid;
        new_node->nodeid = QN->nodeid + 400;
        for(i = 0; i < MAXNODETABLES; i++)
          new_node->relations[0] = 0;
        new_node->relations[0] = QN->relations[1];
        QN->relations[1] = 0;
        new_node->left = QN->right;
        QN->right = new_node;
        new_node->right = 0;
        new_node->child = RIGHTCHILD;
        if (new_node->left != 0)
          new_node->left->parent_nodeid = new_node->nodeid;
        j = QN->attributes->num_attributes();
        new_node->attributes = new Attribute();
        new_node->where_expr = new Expression();
        new_node->join_expr = new Expression();
        if ((j == 1) &&
            (strcasecmp("*", ((Field *)QN->attributes->get_attribute(0))->field_name)==0))
        {
          new_node->attributes = new Attribute();
          new_node->attributes->add_attribute(j, QN->attributes->get_attribute(0));
          QN->attributes->remove_attribute(0);
        }
        else
        {
          Attribute *attribs = 0;
          Item * attr;
          int ii = 0;
          int jj = 0;
          attribs = new Attribute();
          for (i = 0; i < (int)new_node->relations[0]->table->s->fields; i++)
          {
            Field *f = new_node->relations[0]->table->field[i];
            attribs->add_attribute(true, (Item *)f);
            if (j == 0)
            {
              new_node->attributes->hide_attribute((Item *)f, true);
            }
          }
          j = attribs->num_attributes();
          new_node->attributes = new Attribute();
          for (i = 0; i < j; i++)
          {
            attr = attribs->get_attribute(i);
            jj = QN->attributes->index_of(
              (char *)((Field *)attr)->table->s->table_name.str, 
              (char *)((Field *)attr)->field_name);
            if (jj > -1)
            {
              new_node->attributes->add_attribute(ii, attr);
              ii++;
              QN->attributes->remove_attribute(jj);
            }
            else if (find_attr_in_expr(QN->join_expr, 
              (char *)((Field *)attr)->table->s->table_name.str, 
              (char *)((Field *)attr)->field_name)) 
            {
              new_node->attributes->add_attribute(ii, attr);
              new_node->attributes->hide_attribute(attr, true);
              ii++;
            }
          }
        }
      }
    }
    split_project_with_join(QN->left);
    split_project_with_join(QN->right);
  }
  DBUG_RETURN(0);
}

/*
  Find a table in the tree.

  SYNOPSIS
    find_table_in_tree()
    query_node *QN IN the node to operate on
    char *tbl IN the table to locate

  DESCRIPTION
    This method looks for the table name passed in the expressions
    of the tree nodes.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::find_table_in_tree(query_node *QN, char *tbl)
{
  DBUG_ENTER("find_table_in_tree");
  bool found = false;

  if (QN != 0)
    if (QN->where_expr->num_expressions() > 0)
      if (find_table_in_expr(QN->where_expr, tbl))
        found = true;
  if (!found)
    found = find_table_in_tree(QN->left, tbl);
  if (!found)
    found = find_table_in_tree(QN->right, tbl);
  DBUG_RETURN(found);
}

/*
  Find a table in the expression.

  SYNOPSIS
    find_table_in_expr()
    Expression *expr IN the expression to search
    char *tbl IN the table to locate

  DESCRIPTION
    This method looks for the table name passed in the expressions
    specified.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::find_table_in_expr(Expression *expr, char *tbl)
{
  DBUG_ENTER("find_table_in_expr");
  DBUG_RETURN(expr->has_table(tbl));
}

/*
  Find an attribute in the expression.

  SYNOPSIS
    find_attr_in_expr()
    Expression *expr IN the expression to search
    char *tbl IN the table to locate
    char *value IN the name of the attribute

  DESCRIPTION
    This method looks for the attribute passed in the expression
    specified.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::find_attr_in_expr(Expression *expr, char *tbl, char *value)
{
  DBUG_ENTER("find_attr_in_expr");
  DBUG_RETURN(expr->index_of(tbl, value) > 0);
}

/*
  Prepare the query tree for execution.

  SYNOPSIS
    prepare()
    query_node *qn IN the operational node in the query tree.

  DESCRIPTION
    This method is used to open all of the relations used in the query and
    to open the pipeline for execution.

  NOTES
    Walks the tree and start all of the iterators.
    Call this first for each query run.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::prepare(query_node *qn)
{
  int i;
  int ndx;

  DBUG_ENTER("prepare");
  if (qn == root)
  {
    lbuff = NULL;
    rbuff = NULL;
  }
  if (qn != NULL)
  {
    qn->preempt_pipeline = true;  
    if (qn->node_type == (Query_tree::query_node_type) 1 )
    {
      for (i = 0; i < MAXNODETABLES; i++)
        if (qn->relations[i] != NULL)
        {
          ndx = find_index_in_expr(qn->where_expr, qn->relations[i]->table_name);
          if (ndx > -1)
            qn->relations[i]->table->file->ha_index_init(ndx, true);
          else
            qn->relations[i]->table->file->ha_rnd_init(true);
        }
    }
    else
      for (i = 0; i < MAXNODETABLES; i++)
        if (qn->relations[i] != NULL)
            qn->relations[i]->table->file->ha_rnd_init(true);
    prepare(qn->left);
    prepare(qn->right);
  }
  DBUG_RETURN(0);
}

/*
  Shutdown the reads after execution.

  SYNOPSIS
    cleanup()
    query_node *qn IN the operational node in the query tree.

  DESCRIPTION
    This method is used to open all of the relations used in the query and
    to open the pipeline for execution.

  NOTES
    Walks the tree and closes all of the iterators.
    Call this last for each query run.

  RETURN VALUE
    Success = 0
    Failed = 1
*/
int Query_tree::cleanup(query_node *qn)
{
  int i;

  DBUG_ENTER("cleanup");
  if (qn != NULL)
  {
      for (i = 0; i < MAXNODETABLES; i++)
        if (qn->relations[i] != NULL)
            qn->relations[i]->table->file->ha_index_or_rnd_end();
    cleanup(qn->left);
    cleanup(qn->right);
  }
  DBUG_RETURN(0);
}

/*
  Get the next tuple (row) in the result set.

  SYNOPSIS
    Eof()
    query_node *qn IN the operational node in the query tree.

  DESCRIPTION
    This method is used to get the next READ_RECORD from the pipeline.
    The idea is to call prepare() after you've validated the query then call
    get_next to get the first tuple in the pipeline.

  RETURN VALUE
    Success = next tuple in the result set
    Failed = NULL
*/
READ_RECORD *Query_tree::get_next(query_node *qn)
{
  READ_RECORD *next_tup = NULL;
  DBUG_ENTER("get_next");

  /* 
    For each of the possible node types, perform the query operation
    by calling the method for the operation. These implement a very
    high-level abstraction of the operation. The real work is left 
    to the methods.
  */
  switch (qn->node_type)
  {
    /* placeholder for exercises... */
    case Query_tree::qntDistinct :
      break;

    /* placeholder for exercises... */
    case Query_tree::qntUndefined :
      break;

    /* placeholder for exercises... */
    case Query_tree::qntSort :
      if (qn->preempt_pipeline)
        qn->preempt_pipeline = false;
      break;

    /* 
      For restrict, get a row (tuple) from the table and 
      call the do_restrict method looping until a row is returned
      (data matches conditions), then return result to main loop 
      in DBXP_select_command.
    */
    case Query_tree::qntRestrict :
      do
      {
        /* if there is a child, get row from child */
        if (qn->left != NULL)
          next_tup = get_next(qn->left);

        /* else get the row from the table stored in this node */
        else
        {
          /* create space for the record buffer */
          if (next_tup == NULL)
            next_tup = (READ_RECORD *) my_malloc(sizeof(READ_RECORD),
                                              MYF(MY_ZEROFILL | MY_WME));
          next_tup->rec_buf = (uchar *) my_malloc(qn->relations[0]->table->s->rec_buff_length,
                                            MYF(MY_ZEROFILL | MY_WME));

          /* read row from table (storage handler */
          qn->eof[0] = qn->relations[0]->table->file->ha_rnd_next(next_tup->rec_buf);
   
          /* check for end of file */
          if (qn->eof[0] != HA_ERR_END_OF_FILE)
            qn->eof[0] = false;
          else
          {
            qn->eof[0] = true;
            next_tup = NULL;
          }
        }

        /* if there is a row, call the do_restrict method */
        if (next_tup)
          if(!do_restrict(qn, next_tup))
          {
            /* if no row to return, free memory used */
            my_free(next_tup->rec_buf);
            my_free(next_tup);
            next_tup = NULL;
          }
      } while ((next_tup == NULL) && !Eof(qn));
      break;

    /* 
      For project, get a row (tuple) from the table and 
      call the do_project method. If successful, 
      return result to main loop in DBXP_select_command.
    */
    case Query_tree::qntProject :
      /* if there is a child, get row from child */
      if (qn->left != NULL)
      {
        next_tup = get_next(qn->left);
        if (next_tup)
          if (!do_project(qn, next_tup))
          {
            /* if no row to return, free memory used */
            my_free(next_tup->rec_buf);
            my_free(next_tup);
            next_tup = NULL;
          }
      }

      /* else get the row from the table stored in this node */
      else  
      {
        /* create space for the record buffer */
        if (next_tup == NULL)
          next_tup = (READ_RECORD *) my_malloc(sizeof(READ_RECORD),
                                            MYF(MY_ZEROFILL | MY_WME));
        next_tup->rec_buf = (uchar *) my_malloc(qn->relations[0]->table->s->rec_buff_length + 20,
                                          MYF(MY_ZEROFILL | MY_WME));

        /* read row from table (storage handler) */
        qn->eof[0] = qn->relations[0]->table->file->ha_rnd_next(next_tup->rec_buf);
   
        /* check for end of file */
        if (qn->eof[0] != HA_ERR_END_OF_FILE)
        {
          qn->eof[0] = false;
        }
        else
        {
          qn->eof[0] = true;
          next_tup = NULL;
        }

        /* if there is a row, call the do_project method */
        if (next_tup)
        {
          if (!do_project(qn, next_tup))
          {
            /* no row to return, free memory used */
            my_free(next_tup->rec_buf);
            my_free(next_tup);
            next_tup = NULL;
          }
        }
      }
      break;

    /* 
      For join, loop until either a row is returned from the 
      do_join method or we are at end of file for both tables. 
      If successful (data matches conditions),
      return result to main loop in DBXP_select_command.
    */
    case Query_tree::qntJoin :
      do
      {
        if (next_tup)
        {
          /* if no row to return, free memory used */
          my_free(next_tup->rec_buf);
          my_free(next_tup);
          next_tup = NULL;
        }
        next_tup = do_join(qn);
      }
      while ((next_tup == NULL) && !Eof(qn));
      break;
  }
  DBUG_RETURN(next_tup);
}

/*
  Check for end of file.

  SYNOPSIS
    Eof()
    query_node *qn IN the operational node in the query tree.

  DESCRIPTION
    This method is used to determine if the query is complete and the
    pipeline is empty (true). While false, this indicates either the
    relations in the query are not at EOF or the pipeline is not empty.

  NOTES
    Premise: Nodes can have at most 2 relations.
    The concept here is the left relation is in pos 0
    and the right relation is in pos 1. In the case that
    the left child exists, the left relation at pos 0 is
    undefined. Likewise, if the right child exists, the
    right relation at pos 1 is undefined.

    For restrict & project, there should only ever be one
    relation coming from the left child because the query
    optimizer is left-child biased.

  RETURN VALUE
    Success = true: indicates pipeline is empty and query is complete
    Failed = false
*/
bool Query_tree::Eof(query_node *qn)
{
  bool    end_of_file = true;
  bool    end_of_file_left = true;
  bool    end_of_file_right = true;

  DBUG_ENTER("Eof");
  switch (qn->node_type)
  {
    case Query_tree::qntSort :
      break;
    case Query_tree::qntDistinct :  
      if (qn->relations[2] == NULL)
        end_of_file = false;
      else
        end_of_file = qn->eof[2];
      break;
    case Query_tree::qntUndefined :  
      end_of_file = true;
      break;
    case Query_tree::qntRestrict :  
      if (qn->left != NULL)
        end_of_file = Eof(qn->left);
      else
        end_of_file = qn->eof[0];
      break;
    case Query_tree::qntProject :
    {
      if (qn->left != NULL)
        end_of_file = Eof(qn->left);
      else
        end_of_file = qn->eof[0];
      break;
    }
    case Query_tree::qntJoin :
      if ((qn->join_type == jnINNER) || (qn->join_type == jnLEFTOUTER) ||
        (qn->join_type == jnRIGHTOUTER) || (qn->join_type == jnFULLOUTER))
      {
        if (qn->preempt_pipeline)
          end_of_file = false;
        else
        {
          end_of_file_left = qn->eof[2];
          end_of_file_right = qn->eof[3];
          end_of_file = end_of_file_left || end_of_file_right;
          if (end_of_file)
          {
            record_buff *n;
            /*
              clean up buffers
            */
            left_record_buffer_ptr = left_record_buff;
            while (left_record_buffer_ptr != NULL)
            {
              n = left_record_buffer_ptr;
              my_free(n->field_ptr);
              left_record_buffer_ptr = left_record_buffer_ptr->next;
              my_free(n);
            }
            right_record_buffer_ptr = right_record_buff;
            while (right_record_buffer_ptr != NULL)
            {
              n = right_record_buffer_ptr;
              my_free(n->field_ptr);
              right_record_buffer_ptr = right_record_buffer_ptr->next;
              my_free(n);
            }
          }
        }
      }
      else if ((qn->join_type == jnUNION) || (qn->join_type == jnINTERSECT))
      {
        if (qn->preempt_pipeline)
          end_of_file = false;  
        else          
          end_of_file = qn->eof[2];
      }
      else if (qn->join_type == jnCROSSPRODUCT)  
      {
        if (qn->left != NULL)
          end_of_file = Eof(qn->left);
        else
          end_of_file = qn->eof[0];
      }
      else
      {
        if (qn->left != NULL)
          end_of_file_left = Eof(qn->left);
        else
          end_of_file_left = qn->eof[0];
        if (qn->right != NULL)
          end_of_file_right = Eof(qn->right);
        else
          end_of_file_right = qn->eof[1];
        end_of_file = end_of_file_left && end_of_file_right;
      }
      break;
  }
  DBUG_RETURN(end_of_file);
}

/*
  Perform restrict operation.

  SYNOPSIS
    do_restrict()
    query_node *qn IN the operational node in the query tree.
    READ_RECORD *t -- the tuple to apply the operation to.

  DESCRIPTION
    This method performs the relational model operation entitled "restrict".
    This operation is a narrowing of the result set horizontally by
    satisfying the expressions listed in the where clause of the SQL
    statement being executed.

  RETURN VALUE
    Success = true
    Failed = false
*/
bool Query_tree::do_restrict(query_node *qn, READ_RECORD *t)
{
  bool found = false;

  DBUG_ENTER("do_restrict");
  if (qn != NULL)
  {
    /* 
       If the left table isn't NULL, copy the record buffer from
       the table into the record buffer of the relations class.
       This completes the read from the storage engine and now
       provides the data for the projection (which is accomplished
       in send_data().
 
       Lastly, evaluate the where clause. If the where clause 
       evaluates to true, we keep the record else we discard it.
    */
    if (qn->relations[0] != NULL)
      memcpy((uchar *)qn->relations[0]->table->record[0], (uchar *)t->rec_buf, 
        qn->relations[0]->table->s->rec_buff_length);
    if (qn->where_expr != NULL)
      found = qn->where_expr->evaluate(qn->relations[0]->table);
  }
  DBUG_RETURN(found);
}

/*
  Perform project operation.

  SYNOPSIS
    do_project()
    query_node *qn IN the operational node in the query tree.
    READ_RECORD *t -- the tuple to apply the operation to.

  DESCRIPTION
    This method performs the relational model operation entitled "project".
    This operation is a narrowing of the result set vertically by
    restricting the set of attributes in the output tuple.

  NOTES
    Returns 0 (null) if no tuple satisfies child operation (does NOT indicate
    the end of the file or end of query operation. Use Eof() to verify.

  RETURN VALUE
    Success = new tuple with correct attributes
    Failed = NULL
*/
READ_RECORD *Query_tree::do_project(query_node *qn, READ_RECORD *t)
{
  DBUG_ENTER("do_project");
  if (t != NULL)
  {
    if (qn == root)

      /* 
         If the left table isn't NULL, copy the record buffer from
         the table into the record buffer of the relations class.
         This completes the read from the storage engine and now
         provides the data for the projection (which is accomplished
         in send_data().
      */
      if (qn->relations[0] != NULL)
        memcpy((uchar *)qn->relations[0]->table->record[0], 
          (uchar *)t->rec_buf, 
          qn->relations[0]->table->s->rec_buff_length);
  }
  DBUG_RETURN(t);
}

/*
  Perform join operation.

  SYNOPSIS
    do_join()
    query_node *qn IN the operational node in the query tree.
    READ_RECORD *t -- the tuple to apply the operation to.

  DESCRIPTION
    This method performs the relational model operation entitled 
    "join". This operation is the combination of two relations to 
    form a composite view. This algorithm implements ALL variants
    of the join operation.

  NOTES
    Returns 0 (null) if no tuple satisfies child operation (does 
    NOT indicate the end of the file or end of query operation. 
    Use Eof() to verify.

  RETURN VALUE
    Success = new tuple with correct attributes
    Failed = NULL
*/
READ_RECORD *Query_tree::do_join(query_node *qn)
{
  READ_RECORD *next_tup=0;
  int i;
  TABLE *ltable = NULL;
  TABLE *rtable = NULL;
  Field *fright = NULL;
  Field *fleft = NULL;
  record_buff *lprev=0;
  record_buff *rprev=0;
  expr_node *expr;

  DBUG_ENTER("do_join");
  if (qn == NULL)
    DBUG_RETURN(NULL);

  /* check join type because some joins require other processing */
  switch (qn->join_type)
  {
    case (jnUNKNOWN) :
      break;
    case (jnINNER) :  
    case (jnLEFTOUTER) :  
    case (jnRIGHTOUTER) :
    case (jnFULLOUTER) :
    {

      /*
        preempt_pipeline == true means we need to stop the pipeline
        and sort the incoming rows. We do that by making an in-memory
        copy of the record buffers stored in left_record_buff and
        right_record_buff
      */
      if (qn->preempt_pipeline)
      {
        left_record_buff = NULL;
        right_record_buff = NULL;
        next_tup = NULL;

        /* Build buffer for tuples from left child. */
        do
        {
          /* if left child exists, get row from it */
          if (qn->left != NULL)
            lbuff = get_next(qn->left);

          /* else, read the row from the table (the storage handler */
          else
          {
            /* 
               Create space for the record buffer and 
               store pointer in lbuff
            */
            lbuff = (READ_RECORD *) my_malloc(sizeof(READ_RECORD),
                                      MYF(MY_ZEROFILL | MY_WME));
            lbuff->rec_buf = 
              (uchar *) my_malloc(qn->relations[0]->table->s->rec_buff_length,
                                      MYF(MY_ZEROFILL | MY_WME));

            /* check for end of file. Store result in eof array */
            qn->eof[0] = 
              qn->relations[0]->table->file->ha_rnd_next(lbuff->rec_buf);
            if (qn->eof[0] != HA_ERR_END_OF_FILE)
              qn->eof[0] = false;
            else
            {
              lbuff = NULL;
              qn->eof[0] = true;
            }
          }
          /* if the left buffer is not null, get a new row from table */
          if (lbuff != NULL)
          {
            /* we need the table information for processing fields */
            if (qn->left == NULL)
              ltable = qn->relations[0]->table;
            else
              ltable = get_table(qn->left);
            if (ltable != NULL)
              memcpy((uchar *)ltable->record[0], (uchar *)lbuff->rec_buf, 
                ltable->s->rec_buff_length);

            /* get the join expression */
            expr = qn->join_expr->get_expression(0);
            for (Field **field = ltable->field; *field; field++)
              if (strcasecmp((*field)->field_name, ((Field *)expr->left_op)->field_name)==0)
                fleft = (*field);
      
            /* 
               If field was found, add the row to the in-memory buffer
               ordered by the join column.
            */
            if ((fleft != NULL) && (!fleft->is_null()))
              insertion_sort(true, fleft, lbuff);
          }
        } while (lbuff != NULL);
        /* Build buffer for tuples from right child. */
        do
        {
          /* if right child exists, get row from it */
          if (qn->right != NULL)
            rbuff = get_next(qn->right);

          /* else, read the row from the table (the storage handler) */
          else
          {
            /* 
               Create space for the record buffer and 
               store pointer in rbuff
            */
            rbuff = (READ_RECORD *) my_malloc(sizeof(READ_RECORD),
                                      MYF(MY_ZEROFILL | MY_WME));
            rbuff->rec_buf = 
              (uchar *) my_malloc(qn->relations[0]->table->s->rec_buff_length,
                                      MYF(MY_ZEROFILL | MY_WME));

            /* check for end of file. Store result in eof array */
            qn->eof[1] = 
              qn->relations[1]->table->file->ha_rnd_next(rbuff->rec_buf);
            if (qn->eof[1] != HA_ERR_END_OF_FILE)
              qn->eof[1] = false;
            else
            {
              rbuff = NULL;
              qn->eof[1] = true;
            }
          }
          /* if the right buffer is not null, get a new row from table */
          if (rbuff != NULL)
          {
            /* we need the table information for processing fields */
            if (qn->right == NULL)
              rtable = qn->relations[1]->table;
            else
              rtable = get_table(qn->right);
            if (rtable != NULL)
              memcpy((uchar *)rtable->record[0], (uchar *)rbuff->rec_buf, 
                rtable->s->rec_buff_length);

            /* get the join expression */
            expr = qn->join_expr->get_expression(0);
            for (Field **field = rtable->field; *field; field++)
              if (strcasecmp((*field)->field_name, ((Field *)expr->right_op)->field_name)==0)
                fright = (*field);
      
            /* 
               If field was found, add the row to the in-memory buffer
               ordered by the join column.
            */
            if ((fright != NULL) && (!fright->is_null()))
              insertion_sort(false, fright, rbuff);
          }
        } while (rbuff != NULL);
        left_record_buffer_ptr = left_record_buff;
        right_record_buffer_ptr = right_record_buff;
        qn->preempt_pipeline = false;
      }
      /* 
        This is where the actual join code begins.
        We get a tuple from each table and start the compare.
      */

      /* 
         if lbuff is null and the left record buffer has data
         get the row from the buffer
      */
      if ((lbuff == NULL) && (left_record_buffer_ptr != NULL))
      {
        lbuff = left_record_buffer_ptr->record;
        lprev = left_record_buffer_ptr;
        left_record_buffer_ptr = left_record_buffer_ptr->next;
      }

      /* 
         if rbuff is null and the right record buffer has data
         get the row from the buffer
      */
      if ((rbuff == NULL) && (right_record_buffer_ptr != NULL))
      {
        rbuff = right_record_buffer_ptr->record;
        rprev = right_record_buffer_ptr;
        right_record_buffer_ptr = right_record_buffer_ptr->next;
      }

      /* 
        if the left buffer was null, check to see if a row is
        available from left child.
      */
      if (ltable == NULL)
      {
        if (qn->left == NULL)
          ltable = qn->relations[0]->table;
        else
          ltable = get_table(qn->left);
      }
      /* 
        if the right buffer was null, check to see if a row is
        available from right child.
      */
      if (rtable == NULL)
      {
        if (qn->right == NULL)
          rtable = qn->relations[1]->table;
        else
          rtable = get_table(qn->right);
      }
      /*
        If there are two rows to compare, copy the record buffers
        to the table record buffers. This transfers the data
        from the internal buffer to the record buffer. It enables
        us to reuse the MySQL code for manipulating fields.
      */
      if ((lbuff != NULL) && (rbuff != NULL))
      {
        memcpy((uchar *)ltable->record[0], (uchar *)lbuff->rec_buf, 
          ltable->s->rec_buff_length);
        memcpy((uchar *)rtable->record[0], (uchar *)rbuff->rec_buf, 
          rtable->s->rec_buff_length);

        /* evaluate the join condition */
        i = qn->join_expr->compare_join(qn->join_expr->get_expression(0), 
          ltable, rtable);

        /* if there is a match...*/
        if (i == 0)
        {
          /* return the row in the next_tup pointer */
          next_tup = lbuff;
 
          /* store next rows from buffer (already advanced 1 row) */
          record_buff *left = left_record_buffer_ptr;
          record_buff *right = right_record_buffer_ptr;

          /* 
             Check to see if either buffer needs to be rewound to
             allow us to process many rows on one side to one row
             on the other
          */
          check_rewind(left_record_buffer_ptr, lprev,
            right_record_buffer_ptr, rprev);

          /* set poointer to null to force read on next loop */
          lbuff = NULL;
          rbuff = NULL;

          /*
            If the left buffer has been changed and if the 
            buffer is not at the end, set the buffer to the next row.
          */
          if (left != left_record_buffer_ptr)
          {
            if (left_record_buffer_ptr != NULL)
            {
              lbuff = left_record_buffer_ptr->record;
            }
          }

          /*
            If the right buffer has been changed and if the 
            buffer is not at the end, set the buffer to the next row.
          */
          if (right != right_record_buffer_ptr)
          {
            if (right_record_buffer_ptr != NULL)
            {
              rbuff = right_record_buffer_ptr->record;
            }
          }

          /* Now check for end of file and save results in eof array */
          if (left_record_buffer_ptr == NULL)
            qn->eof[2] = true;
          else
            qn->eof[2] = false;
          if (right_record_buffer_ptr == NULL)
            qn->eof[3] = true;
          else
            qn->eof[3] = false;
        }

        /* if the rows didn't match...*/
        else
        {
          /* get next rows from buffers (already advanced) */
          record_buff *left = left_record_buffer_ptr;
          record_buff *right = right_record_buffer_ptr;

          /* 
             Check to see if either buffer needs to be rewound to
             allow us to process many rows on one side to one row
             on the other. The results of this rewind must be
             saved because there was no match and we may have to
             reuse one or more of the rows.
          */
          check_rewind(left_record_buffer_ptr, lprev,
            right_record_buffer_ptr, rprev);

          /*
            If the left buffer has been changed and if the 
            buffer is not at the end, set the buffer to the next row
            and copy the data into the record buffer/
          */
          if (left != left_record_buffer_ptr)
          {
            if (left_record_buffer_ptr != NULL)
            {
              memcpy((uchar *)ltable->record[0], 
                (uchar *)left_record_buffer_ptr->record->rec_buf, 
                ltable->s->rec_buff_length);
              lbuff = left_record_buffer_ptr->record;
            }
          }

          /*
            If the right buffer has been changed and if the 
            buffer is not at the end, set the buffer to the next row
            and copy the data into the record buffer/
          */
          if (right_record_buffer_ptr != NULL)
            if ((right_record_buffer_ptr->next == NULL) &&
               (right_record_buffer_ptr->prev == NULL))
               lbuff = NULL;
          if (right != right_record_buffer_ptr)
          {
            if (right_record_buffer_ptr != NULL)
            {
              memcpy((uchar *)rtable->record[0], 
                (uchar *)right_record_buffer_ptr->record->rec_buf, 
                rtable->s->rec_buff_length);
              rbuff = right_record_buffer_ptr->record;
            }
          }

          /* Now check for end of file and save results in eof array */
          if (left_record_buffer_ptr == NULL)
            qn->eof[2] = true;
          else
            qn->eof[2] = false;
          if (right_record_buffer_ptr == NULL)
            qn->eof[3] = true;
          else
            qn->eof[3] = false;
            
          next_tup = NULL;
        }
      }
      else
      {
        next_tup = NULL; /* at end, return null */
      }
      break;
    }

    /* placeholder for exercise... */
    case (jnCROSSPRODUCT) :
    {
      break;
    }
    /*
      placeholder for exercises... 
      Union and intersect are mirrors of each other -- same code will
      work for both except the dupe elimination/inclusion part (see below)
    */
    case (jnUNION) :
    case (jnINTERSECT) :
    {
      break;
    }
  }
  DBUG_RETURN(next_tup);
}

/*
  Locates an index if a key field exists in the expression.

  SYNOPSIS
    find_index_in_expr()
    query_node *qn IN the operational node in the query tree.

  DESCRIPTION
    This method is used to locate a key in the expressions. If found,
    the number of the index is returned.

  NOTES
    Walks the tree and start all of the iterators.
    Call this first for each query run.

  RETURN VALUE
    Success = int index number
    Failed = -1
*/
int Query_tree::find_index_in_expr(Expression *e, char *tbl)
{
  int ndx = -1;

  DBUG_ENTER("find_index_in_expr");
  DBUG_RETURN(ndx);
}

/*
  Locates the table from the subtree.

  SYNOPSIS
    get_table()
    query_node *qn IN the operational node in the query tree.

  DESCRIPTION
    This method is used to locate a table from the subtree.

  RETURN VALUE
    Success = int index number
    Failed = -1
*/
TABLE *Query_tree::get_table(query_node *qn)
{
  TABLE *t = NULL;
  DBUG_ENTER("get_table");
  if (qn != NULL)
  {
    if (qn->left == NULL)
      t = qn->relations[0]->table;
    else
      DBUG_RETURN(get_table(qn->left));
    if (t == NULL)
    {
      if (qn->right == NULL)
        t = qn->relations[1]->table;
      else
        DBUG_RETURN(get_table(qn->right));
    }
  }
  DBUG_RETURN(t);
}

/*
  Sort the relation buffer on the following attributes...

  SYNOPSIS
    insertion_sort()
    record_buff *buff IN OUT the record buffer you want to insert into.
    Field *field IN the field for the sort.
    READ_RECORD *rcd IN the record buffer to insert.

  DESCRIPTION
    This method is used to create a buffer of ordered record buffers.

  NOTES
    To initialize a new buffer, pass NULL for field value then call
    again for each record inserted.

  RETURN VALUE
    Success = int index number
    Failed = -1
*/
int Query_tree::insertion_sort(bool left, 
                               Field *field, 
                               READ_RECORD *rcd)
{
  int i;
  record_buff *b_ptr;
  record_buff *n_ptr;
  record_buff *p_ptr;
  record_buff *buff;
  bool found = false;
  char buffer[MAX_FIELD_WIDTH];
  String str(buffer,sizeof(buffer),&my_charset_bin);

  DBUG_ENTER("sort");
  if ((field == NULL) || (rcd == NULL))
    DBUG_RETURN(-1);

  /* Check for None field - ignore these! */
  field->val_str(&str);
  if (strncasecmp(str.ptr(), "NONE", 4)==0)
    DBUG_RETURN(-1);

  if (left)
    buff = left_record_buff;
  else
    buff = right_record_buff;
  if (buff == NULL)
  {
    buff = (record_buff *)my_malloc(sizeof(record_buff), MYF(MY_ZEROFILL |MY_WME));
    field->val_str(&str);
    buff->field_ptr = (uchar *)my_malloc(str.length(), 
      MYF(MY_ZEROFILL |MY_WME));
    memcpy(buff->field_ptr, str.ptr(), str.length());
    buff->field_length = str.length();
    buff->record = rcd;
    buff->next = NULL;
    buff->prev = NULL;
    if (left)
      left_record_buff = buff;
    else
      right_record_buff = buff;
  }
  else 
  {
    b_ptr = buff;
    while (!found && (b_ptr != NULL))
    {
      field->val_str(&str);
      i = memcmp(str.ptr(), b_ptr->field_ptr, 
        (long)str.length() < b_ptr->field_length ? 
        strlen(field->field_name) : b_ptr->field_length);
      if (i < 0)
      {
        if (buff == b_ptr)
        {
          n_ptr = (record_buff *)my_malloc(sizeof(record_buff), 
            MYF(MY_WME));
          n_ptr->field_ptr = (uchar *)my_malloc(str.length(), 
            MYF(MY_WME));
          memcpy(n_ptr->field_ptr, str.ptr(), str.length());
          n_ptr->field_length = str.length();
          n_ptr->record = rcd;
          n_ptr->prev = NULL;
          buff->prev = n_ptr;
          n_ptr->next = buff;
          buff = n_ptr;
          if (left)
            left_record_buff = n_ptr;
          else
            right_record_buff = n_ptr;
          found = true;
        }
        else
        {
          p_ptr = b_ptr->prev;
          p_ptr->next = (record_buff *)my_malloc(sizeof(record_buff), 
            MYF(MY_WME));
          p_ptr->next->next = b_ptr;
          b_ptr->prev = p_ptr->next;
          p_ptr->next->field_ptr = 
            (uchar *)my_malloc(str.length(), MYF(MY_WME));
          memcpy(p_ptr->next->field_ptr, str.ptr(), str.length());
          p_ptr->next->field_length = str.length();
          p_ptr->next->prev = p_ptr;
          p_ptr->next->record = rcd;
          found = true;
        }
      }
      else if (b_ptr->next == NULL)
      {
        b_ptr->next = (record_buff *)my_malloc(sizeof(record_buff), 
          MYF(MY_WME));
        b_ptr->next->next = NULL;
        b_ptr->next->field_ptr = 
          (uchar *)my_malloc(str.length(), MYF(MY_WME));
        memcpy(b_ptr->next->field_ptr, str.ptr(), str.length());
        b_ptr->next->field_length = str.length();
        b_ptr->next->prev = b_ptr;
        b_ptr->next->record = rcd;
        found = true;
      }
      else
        b_ptr = b_ptr->next;
    }
  }
  DBUG_RETURN(0);
}

/*
  Adjusts pointers to record buffers for join.

  SYNOPSIS
    check_rewind()
    record_buff *cur_left IN the left record buffer
    record_buff *cur_left_prev IN the left record buffer previous
    record_buff *cur_right IN the left record buffer
    record_buff *cur_right_prev IN the left record buffer previous

  DESCRIPTION
    This method is used to push a tuple back into the buffer 
    during a join operation that preempts the pipeline.

  NOTES
    Now, here's where we have to check the next tuple in each
    relation to see if they are the same. If one of them is the
    same and the other isn't, push one of them back.

    We need to rewind if one of the following is true:
    1. The next record in R2 has the same join value as R1
    2. The next record in R1 has the same join value as R2
    3. The next record in R1 has the same join value and R2 is 
       different (or EOF)
    4. The next record in R2 has the same join value and R1 is 
       different (or EOF)

  RETURN VALUE
    Success = int index number
    Failed = -1
*/
int Query_tree::check_rewind(record_buff *cur_left, 
                             record_buff *curr_left_prev,
                             record_buff *cur_right, 
                             record_buff *curr_right_prev)
{
  record_buff *left_rcd_ptr = cur_left;
  record_buff *right_rcd_ptr = cur_right;
  int i;
  DBUG_ENTER("check_rewind");

  /*
    If the next tuple in right record is the same as the present tuple
      AND the next tuple in right record is different, rewind until 
      it is the same
    else
      Push left record back.
  */

  /* if both buffers are at EOF, return -- nothing to do */
  if ((left_rcd_ptr == NULL) && (right_rcd_ptr == NULL))
    DBUG_RETURN(0);

  /* if the currently processed record is null, get the one before it */
  if (cur_right == NULL)
    right_rcd_ptr = curr_right_prev;

  /* 
    if left buffer is not at end, check to see 
    if we need to rewind right buffer 
  */
  if (left_rcd_ptr != NULL)
  {
    /* compare the join conditions to check order */
    i = memcmp(left_rcd_ptr->field_ptr, right_rcd_ptr->field_ptr, 
      left_rcd_ptr->field_length < right_rcd_ptr->field_length ? 
      left_rcd_ptr->field_length : right_rcd_ptr->field_length);

    /*
      i == 0 means the rows are the same. In this case, we need to
      check to see if we need to advance or rewind the right buffer.
    */
    if (i == 0)
    {
      /*
        If there is a next row in the right buffer, check to see
        if it matches the left row. If the right row is greater
        than the left row, rewind the right buffer to one previous
        to the current row or until we hit the start.
      */
      if (right_rcd_ptr->next != NULL)
      {
        right_rcd_ptr = right_rcd_ptr->next;
        i = memcmp(left_rcd_ptr->field_ptr, right_rcd_ptr->field_ptr, 
          left_rcd_ptr->field_length < right_rcd_ptr->field_length ? 
          left_rcd_ptr->field_length : right_rcd_ptr->field_length);
        if (i > 0)
        {
          do
          {
            if (right_rcd_ptr->prev != NULL)
            {
              right_rcd_ptr = right_rcd_ptr->prev;
              i = memcmp(left_rcd_ptr->field_ptr, right_rcd_ptr->field_ptr, 
                left_rcd_ptr->field_length < right_rcd_ptr->field_length ? 
                left_rcd_ptr->field_length : right_rcd_ptr->field_length);
            }
          }
          while ((i == 0) && (right_rcd_ptr->prev != NULL));

          /* now advance one more to set pointer to correct location */
          if (right_rcd_ptr->next != NULL)
            right_rcd_ptr = right_rcd_ptr->next;
        }
        /* if no next right row, rewind to previous row */
        else
          right_rcd_ptr = right_rcd_ptr->prev;
      }
      /*
        If there is a next row in the left buffer, check to see
        if it matches the right row. If there is a match and the right
        buffer is not at start, rewind the right buffer to one previous
        to the current row.
      */
      else if (left_rcd_ptr->next != NULL)
      {
        if (right_rcd_ptr->prev != NULL)
        {
          i = memcmp(left_rcd_ptr->field_ptr, right_rcd_ptr->prev->field_ptr, 
            left_rcd_ptr->field_length < right_rcd_ptr->prev->field_length ? 
            left_rcd_ptr->field_length : right_rcd_ptr->prev->field_length);
        }
        if ((i == 0) && (right_rcd_ptr->prev != NULL))
          right_rcd_ptr = right_rcd_ptr->prev;
      }
    }
    /* if the left row is less than right row, rewind right buffer */
    else if (i < 0)
    {
      if (right_rcd_ptr->prev != NULL)
        right_rcd_ptr = right_rcd_ptr->prev;
    }
    /* if the right row is less than the left row, advance right row */
    else
    {
      if (right_rcd_ptr->next != NULL)
        right_rcd_ptr = right_rcd_ptr->next;
    }
  }
  /* 
    Rows don't match so advance the right buffer and check match again.
    if they still match, rewind left buffer.
  */
  else
  {
    if (right_rcd_ptr->next != NULL)
    {
      i = memcmp(curr_left_prev->field_ptr, right_rcd_ptr->field_ptr, 
        curr_left_prev->field_length < right_rcd_ptr->field_length ? 
        curr_left_prev->field_length : right_rcd_ptr->field_length);
      if (i == 0)
        left_rcd_ptr = curr_left_prev;
    }
  }
  /* set buffer pointers to adjusted rows from buffers */
  left_record_buffer_ptr = left_rcd_ptr;
  right_record_buffer_ptr = right_rcd_ptr;
  DBUG_RETURN(0);
}
