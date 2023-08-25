/*-------------------------------------------------------------------------
 *
 * rbtree.h
 *      interface for PostgreSQL generic Red-Black binary tree package
 *
 * Copyright (c) 2009-2023, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *        src/include/lib/rbtree.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RBTREE_H
#define RBTREE_H

#include <ndrstandard.h>

/*
 * RBTNode is intended to be used as the first field of a larger struct,
 * whose additional fields carry whatever payload data the caller needs
 * for a tree entry.  (The total size of that larger struct is passed to
 * rbt_create.)    RBTNode is declared here to support this usage, but
 * callers must treat it as an opaque struct.
 */
typedef struct ndrx_rbt_node
{
    char color;                     /* node's current color, red or black */
    struct ndrx_rbt_node *left;     /* left child, or RBTNIL if none */
    struct ndrx_rbt_node *right;    /* right child, or RBTNIL if none */
    struct ndrx_rbt_node *parent;   /* parent, or NULL (not RBTNIL!) if none */
} ndrx_rbt_node_t;

/* Support functions to be provided by caller */
typedef int (*rbt_comparator) (const ndrx_rbt_node_t *a,
                                const ndrx_rbt_node_t *b, void *arg);
typedef void (*rbt_combiner) (ndrx_rbt_node_t *existing,
                                const ndrx_rbt_node_t *newdata, void *arg);
typedef void (*rbt_freefunc) (ndrx_rbt_node_t *x, void *arg);

/*
 * RBTree control structure
 */
struct ndrx_rbt_tree
{
    ndrx_rbt_node_t *root;            /* root node, or RBTNIL if tree is empty */
    /* Remaining fields are constant after rbt_create */
    /* The caller-supplied manipulation functions */
    rbt_comparator  comparator;
    rbt_combiner    combiner;
    rbt_freefunc    freefunc;
    /* Passthrough arg passed to all manipulation functions */
    void            *arg;
};

/* struct representing a whole tree */
typedef struct ndrx_rbt_tree ndrx_rbt_tree_t;

/* Available tree iteration orderings */
typedef enum ndrx_rbt_order_control
{
    LeftRightWalk,                /* inorder: left child, node, right child */
    RightLeftWalk                 /* reverse inorder: right, node, left */
} ndrx_rbt_order_control_t;

/*
 * RBTreeIterator holds state while traversing a tree.  This is declared
 * here so that callers can stack-allocate this, but must otherwise be
 * treated as an opaque struct.
 */
typedef struct ndrx_rbt_tree_iterator ndrx_rbt_tree_iterator_t;

struct ndrx_rbt_tree_iterator
{
    ndrx_rbt_tree_t     *rbt;
    ndrx_rbt_node_t     *(*iterate) (ndrx_rbt_tree_iterator_t *iter);
    ndrx_rbt_node_t     *last_visited;
    int                 is_over;
};

extern ndrx_rbt_tree_t *ndrx_rbt_create(rbt_comparator comparator,
                                            rbt_combiner combiner,
                                            rbt_freefunc freefunc,
                                            void *arg);

extern ndrx_rbt_tree_t *ndrx_rbt_init(ndrx_rbt_tree_t *tree,
                                    rbt_comparator comparator,
                                    rbt_combiner combiner,
                                    rbt_freefunc freefunc,
                                    void *arg);

extern ndrx_rbt_node_t *ndrx_rbt_find(ndrx_rbt_tree_t *rbt, 
                                        const ndrx_rbt_node_t *data);
extern ndrx_rbt_node_t *ndrx_rbt_find_great(ndrx_rbt_tree_t *rbt, 
                                            const ndrx_rbt_node_t *data, 
                                            int equal_match);
extern ndrx_rbt_node_t *ndrx_rbt_find_less(ndrx_rbt_tree_t *rbt, 
                                            const ndrx_rbt_node_t *data, 
                                            int equal_match);
extern ndrx_rbt_node_t *ndrx_rbt_leftmost(ndrx_rbt_tree_t *rbt);

extern ndrx_rbt_node_t *ndrx_rbt_insert(ndrx_rbt_tree_t *rbt, 
                                        ndrx_rbt_node_t *data, int *isNew);
extern void ndrx_rbt_delete(ndrx_rbt_tree_t *rbt, ndrx_rbt_node_t *node);

extern void ndrx_rbt_begin_iterate(ndrx_rbt_tree_t *rbt, 
                                    ndrx_rbt_order_control_t ctrl,
                                    ndrx_rbt_tree_iterator_t *iter);

extern ndrx_rbt_node_t *ndrx_rbt_iterate(ndrx_rbt_tree_iterator_t *iter);

#endif                            /* RBTREE_H */
