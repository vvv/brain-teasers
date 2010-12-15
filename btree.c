#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "btree.h"
#include "util.h"

static size_t
_index(uint32_t k, const uint32_t arr[], size_t size)
{
	size_t i;
	for (i = 0; i < size; ++i) { /* XXX use binary search */
		if (arr[i] >= k)
			break;
	}
	return i;
}

/* Backtrace element */
struct Frame {
	struct list_head h;

	struct Btree_Node *node; /* Pointer to the node */
	size_t sid; /* Index of the node's son that we descend into */
};

static void
push(struct list_head *bt, struct Btree_Node *node, size_t sid)
{
	struct Frame *new = xmalloc(sizeof(*new));
	INIT_LIST_HEAD(&new->h);
	new->node = node;
	new->sid = sid;

	list_add(&new->h, bt);
}

static struct Btree_Node *
pop(struct list_head *bt)
{
	assert(!list_empty(bt));

	struct Frame *x = list_entry(bt->next, struct Frame, h);
	struct Btree_Node *node = x->node;

	__list_del(bt, bt->next->next);
	free(x);

	return node;
}

void
btree_destroy(struct Btree_Head *head)
{
	if (head->root == NULL || head->height == 0) {
		free(head->root);
		return;
	}

	struct Btree_Node *node = head->root;
	struct Btree_Node *son;
	uint8_t h = head->height - 1;
	LIST_HEAD(bt);

	for (;;) {
		if (h == 0) {
			while (node->size != 0)
				free(node->u.sons[node->size--]);
			free(*node->u.sons);
		} else {
			son = node->u.sons[node->size];

			if (node->size == 0)
				*node->u.sons = NULL;
			else
				--node->size;

			if (son != NULL) {
				push(&bt, node, (size_t) -1);
				node = son;
				--h;
				continue;
			}
		}

		free(node);

		if (list_empty(&bt))
			return;

		node = pop(&bt);
		++h;
	}
}

/*
 * Find the leaf that would hold the given key,
 *
 * This function ignores the fact that the leaf may be full.
 *
 * @bt: backtrace of interior nodes -- the path from the root to the
 *      target leaf, excluding the latter
 */
static struct Btree_Node *
target_leaf(const struct Btree_Head *head, uint32_t key, struct list_head *bt)
{
	assert(list_empty(bt));

	struct Btree_Node *x = head->root;

	uint8_t h;
	for (h = head->height; h != 0; --h) {
		const size_t i = _index(key, x->keys, x->size);
		push(bt, x, i);
		x = x->u.sons[i];
	}

	return x;
}

static int
squeeze_left(uint32_t key, struct Btree_Node *node, size_t pos,
	     const struct list_head *bt)
{
	assert(node->size == ARRAY_SIZE(node->keys));

	if (list_empty(bt))
		return -1; /* the only leaf */
	const struct Frame *parent = list_first_entry(bt, struct Frame, h);

	if (parent->sid == 0)
#warning "XXX Left brother availability criteria is incorrect."
		return -1; /* no left brother */
	struct Btree_Node *left = parent->node->u.sons[parent->sid - 1];

	if (left->size == ARRAY_SIZE(left->keys))
		return -1; /* left brother is full */

	if (pos == 0) {
		left->keys[left->size++] = key;
		return 0;
	}

	left->keys[left->size++] = *node->keys;
	if (pos > 1)
		memmove(node->keys, node->keys + 1,
			(pos - 1) * sizeof(*node->keys));
	node->keys[pos - 1] = key;

	parent->node->keys[parent->sid - 1] = left->keys[left->size - 1];
	return 0;
}

static int
squeeze_right(uint32_t key, struct Btree_Node *node, size_t pos,
	      const struct list_head *bt)
{
	assert(node->size == ARRAY_SIZE(node->keys));

	if (list_empty(bt))
		return -1; /* the only leaf */
	const struct Frame *parent = list_first_entry(bt, struct Frame, h);

	if (parent->sid == parent->node->size)
#warning "XXX Right brother availability criteria is incorrect."
		return -1; /* no right brother */
	struct Btree_Node *right = parent->node->u.sons[parent->sid + 1];

	if (right->size == ARRAY_SIZE(right->keys))
		return -1; /* right brother is full */

	memmove(right->keys + 1, right->keys,
		right->size * sizeof(*right->keys));
	++right->size;

	if (pos == ARRAY_SIZE(node->keys)) {
		*right->keys = key;
		return 0;
	}

	*right->keys = node->keys[node->size - 1];
	if (pos < ARRAY_SIZE(node->keys) - 1)
		memmove(node->keys + pos + 1, node->keys + pos,
			(ARRAY_SIZE(node->keys) - pos - 1)
			* sizeof(*node->keys));
	node->keys[pos] = key;

	parent->node->keys[parent->sid] = node->keys[node->size - 1];
	return 0;
}

int
btree_insert(struct Btree_Head *head, uint32_t key)
{
	struct Btree_Node *x;
	if (head->root == NULL) {
		x = xmalloc(sizeof(*x));
		*x->keys = key;
		x->size = 1;
		INIT_LIST_HEAD(&x->u.leaf);
		head->root = x;
		list_add(&x->u.leaf, &head->leaves);
		return 0;
	}

	LIST_HEAD(bt); /* backtrace */
	x = target_leaf(head, key, &bt);
	const size_t i = _index(key, x->keys, x->size);

	if (i < x->size && x->keys[i] == key)
		return -1; /* key exists */

	if (x->size < ARRAY_SIZE(x->keys)) {
		if (i < x->size)
			memmove(x->keys + i + 1, x->keys + i,
				(x->size - i) * sizeof(*x->keys));

		x->keys[i] = key;
		++x->size;

		return 0;
	}

	if (squeeze_left(key, x, i, &bt) == 0 ||
	    squeeze_right(key, x, i, &bt) == 0)
		return 0;

	/* The leaf is full and needs to be splitted. */
	assert(0 == 1); /* XXX not implemented */
}

#if 0 /* XXX ======================================================== */
int
btree_insert(struct Btree_Head *head, uint32_t key)
{
	if (head->height == 0) {
		assert(head->root == NULL);

		struct Btree_Leaf *x = xmalloc(sizeof(*x));
		INIT_LIST_HEAD(&x->h);

		*x->vals = key;
		x->size = 1;

		head->root = x;
		++head->height;
		list_add(&x->h, &head->leaves);

		return 0;
	}

	if (head->height == 1) {
		struct Btree_Leaf *x = head->root;
		size_t i;
		uint32_t *xs = x->vals;

		/* Find location */
		for (i = 0; i < x->size; ++i) {
			if (xs[i] > key)
				break;
			else if (xs[i] == key)
				return -1; /* already present */
		}

		if (x->size < BTREE_2K) { /* no need to split */
			if (i != BTREE_2K - 1)
				memmove(xs + i + 1, xs + i,
					(BTREE_2K - i - 1) * sizeof(*xs));
			xs[i] = key;
			++x->size;
			return 0;
		}

		/* Splitting, pt. 1: create a sibling leaf */
		assert(x->size == BTREE_2K);
		struct Btree_Leaf *y = xmalloc(sizeof(*y));
		INIT_LIST_HEAD(&y->h);
		uint32_t *ys = y->vals;

		x->size = BTREE_K + 1;
		y->size = BTREE_K;
		list_add(&y->h, &x->h);

		if (i <= BTREE_K) {
			memcpy(ys, xs + BTREE_K, BTREE_K * sizeof(*xs));
			if (i != BTREE_K)
				memcpy(xs + i + 1, xs + i,
				       (BTREE_K - i) * sizeof(*xs));
			xs[i] = key;
		} else {
			if (i != BTREE_K + 1)
				memcpy(ys, xs + BTREE_K + 1,
				       (i - BTREE_K - 1) * sizeof(*xs));
			if (i != BTREE_2K)
				memcpy(ys + i - BTREE_K, xs + i,
				       (BTREE_2K - i) * sizeof(*xs));

			ys[i - BTREE_K - 1] = key;
		}

		/* Splitting, pt. 2: create new root */
		struct Btree_Node *root = xmalloc(sizeof(*root));
		root->size = 1;
		root->keys[0] = xs[BTREE_K];
		root->sons[0] = x;
		root->sons[1] = y;

		head->root = root;
		++head->height;

		return 0;
	}

	assert(0 == 1); /* XXX not implemented */
}

100 110 120 130 140 150 160 170  (k=4)
  5  i=0    5 100 110 120 130 - 140 150 160 170  i < k
105  i=1  100 105 110 120 130 - 140 150 160 170  i < k
115  i=2  100 110 115 120 130 - 140 150 160 170  i < k
125  i=3  100 110 120 125 130 - 140 150 160 170  i < k
135  i=4  100 110 120 130 135 - 140 150 160 170  i == k
145  i=5  100 110 120 130 140 - 145 150 160 170  i == k + 1
155  i=6  100 110 120 130 140 - 150 155 160 170  i > k
165  i=7  100 110 120 130 140 - 150 160 165 170  i > k
500  i=8  100 110 120 130 140 - 150 160 170 500  i == 2k

100 110 120 130 140 150  (k=3)
  5  i=0    5 100 110 120 - 130 140 150  i < k
105  i=1  100 105 110 120 - 130 140 150  i < k
115  i=2  100 110 115 120 - 130 140 150  i < k
125  i=3  100 110 120 125 - 130 140 150  i == k
135  i=4  100 110 120 130 - 135 140 150  i == k + 1
145  i=5  100 110 120 130 - 140 145 150  i > k
500  i=6  100 110 120 130 - 140 150 500  i == 2k

100 110 120 130  (k=2)
  5  i=0    5 100 110 - 120 130  i < k
105  i=1  100 105 110 - 120 130  i < k
115  i=2  100 110 115 - 120 130  i == k
125  i=3  100 110 120 - 125 130  i == k + 1
500  i=4  100 110 120 - 130 500  i == 2k

digraph G {
	rankdir=RL;
	node [shape=record];

	root [label="<p0>|38|<p1>|145|<p2>|199|<p3>|241|<p4>"];
	n_1_1 [label="<p0>|8|<p1>|13|<p2>|38|<p3>|X|<p4>"];
	n_1_2 [label="<p0>|83|<p1>|144|<p2>|145|<p3>|X|<p4>"];
	n_1_3 [label="<p0>|149|<p1>|150|<p2>|199|<p3>|201|<p4>"];
	n_1_4 [label="<p0>|212|<p1>|237|<p2>|241|<p3>|X|<p4>"];
	n_1_5 [label="<p0>|242|<p1>|243|<p2>|X|<p3>|X|<p4>"];

	root:p0 -> n_1_1;
	root:p1 -> n_1_2;
	root:p2 -> n_1_3;
	root:p3 -> n_1_4;
	root:p4 -> n_1_5;
}
#endif /* XXX ======================================================= */
