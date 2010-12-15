#ifndef _BTREE_H
#define _BTREE_H

/*
 * B+ tree with overflow.
 *
 * B+ tree [http://en.wikipedia.org/wiki/B%2B_tree] is a variation of
 * B-tree in which all records are stored at the leaf level of the
 * tree; only keys are stored in interior nodes.
 *
 * The definition of B-tree [Acta Informatica (1972), 173-189]:
 *
 * | Let h >= 0 be an integer, k a natural number. A directed tree T
 * | is in the class t(k,h) of _B-trees_ if T is either empty (h = 0)
 * | or has the following properties:
 * |
 * |   i) Each path from the root to any leaf has the same length h,
 * |   also called the _height_ of T, i.e., h = number of nodes in
 * |   path.
 * |
 * |   ii) Each node except the root and the leaves has at least k + 1
 * |   sons. The root is a leaf or has at least two sons.
 * |
 * |   iii) Each node has at most 2k + 1 sons.
 *
 * Insertions and deletions use the idea of ``overflow''
 * [Bayer and McCreight].
 *
 * Our needs are simple: all we need is to store numeric values. Thus
 * there's no need to differentiate keys from values. We store a
 * _value_ of type `unit32_t' in a leaf, and this value is also a
 * _key_ at the same time.
 */

#include <stdint.h>

#include "list.h"

/*
 * The value m = 2k + 1, where k is a natural number from B-tree
 * definition (above), is called the _order_ of B-tree.
 */
#ifdef DEBUG
enum { BTREE_K = 2, BTREE_2K = 2*BTREE_K };
/* enum { BTREE_K = 3, BTREE_2K = 2*BTREE_K }; */
/* enum { BTREE_K = 4, BTREE_2K = 2*BTREE_K }; */
#else
enum { BTREE_K = 25, BTREE_2K = 2*BTREE_K };
#endif

struct Btree_Head {
	struct Btree_Node *root;
	uint8_t height;
	struct list_head leaves;
};

#define BTREE_HEAD(NAME) \
	struct Btree_Head NAME = { NULL, 0, LIST_HEAD_INIT(NAME.leaves) }

struct Btree_Node {
	uint8_t size; /* Number of _keys_ in this node.
		       * Invariant: k < size <= 2*k;
		       * for the root node: 0 < size <= 2*k. */
	uint32_t keys[BTREE_2K];
	union {
		struct Btree_Node *sons[BTREE_2K+1];
		struct list_head leaf;
	} u;
};

/*
 * Insert new key into the tree.
 *
 * A key is also a value in our case.
 *
 * Return values: 0 - success, -1 - error (e.g., attempting to insert
 * existing key).
 */
int btree_insert(struct Btree_Head *head, uint32_t key);

/* Delete the tree, freeing allocated resources */
void btree_destroy(struct Btree_Head *head);

#endif /* _BTREE_H */
