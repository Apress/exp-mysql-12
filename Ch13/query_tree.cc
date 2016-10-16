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
  query_node       *NewQN=0;

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
        {
          if (find_table_in_expr(pNode->where_expr, QN->relations[0]->table_name))
          {
            NewQN->relations[0] = QN->relations[0];
            QN->relations[0] = 0;
          }
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

