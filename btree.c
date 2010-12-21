#include <string.h>
#include <assert.h>

#include "btree.h"
#include "util.h"

enum { KEY_SZ = sizeof(*((struct Btree_Node *) NULL)->keys) };

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
pop(struct list_head *bt, size_t *sid)
{
	assert(!list_empty(bt));

	struct Frame *x = list_entry(bt->next, struct Frame, h);
	struct Btree_Node *node = x->node;
	if (sid != NULL)
		*sid = x->sid;

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

		node = pop(&bt, NULL);
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

static inline uint32_t
max_key(const struct Btree_Node *x)
{
	assert(x->size > 0);
	return x->keys[x->size - 1];
}

static int
squeeze_left(uint32_t k, struct Btree_Node *x, size_t i,
	     const struct list_head *bt)
{
	assert(x->size == BTREE_2K);

	if (list_empty(bt))
		return -1; /* the only leaf */
	const struct Frame *parent = list_first_entry(bt, struct Frame, h);

	if (parent->sid == 0)
#warning "XXX Left brother availability criteria is incorrect."
		return -1; /* no left brother */
	struct Btree_Node *left = parent->node->u.sons[parent->sid - 1];

	if (left->size == BTREE_2K)
		return -1; /* left brother is full */

	if (i == 0) {
		left->keys[left->size++] = k;
		return 0;
	}

	left->keys[left->size++] = *x->keys;
	if (i > 1)
		memmove(x->keys, x->keys + 1, (i - 1) * KEY_SZ);
	x->keys[i - 1] = k;

	parent->node->keys[parent->sid - 1] = max_key(left);
	return 0;
}

static int
squeeze_right(uint32_t k, struct Btree_Node *x, size_t i,
	      const struct list_head *bt)
{
	assert(x->size == BTREE_2K);

	if (list_empty(bt))
		return -1; /* the only leaf */
	const struct Frame *parent = list_first_entry(bt, struct Frame, h);

	if (parent->sid == parent->node->size)
#warning "XXX Right brother availability criteria is incorrect."
		return -1; /* no right brother */
	struct Btree_Node *right = parent->node->u.sons[parent->sid + 1];

	if (right->size == BTREE_2K)
		return -1; /* right brother is full */

	memmove(right->keys + 1, right->keys, right->size * KEY_SZ);
	++right->size;

	if (i == BTREE_2K) {
		*right->keys = k;
		return 0;
	}

	*right->keys = max_key(x);
	if (i < BTREE_2K - 1)
		memmove(x->keys + i + 1, x->keys + i,
			(BTREE_2K - i - 1) * KEY_SZ);
	x->keys[i] = k;

	parent->node->keys[parent->sid] = max_key(x);
	return 0;
}

static inline int
is_full(const struct Btree_Node *x)
{
	return x->size == BTREE_2K;
}

/*
 * Split the leaf and insert the key.
 *
 * @x: leaf to split
 * @k: key to insert
 * @i: position in x->keys array where the key would go if the array
 *     was not full
 */
static void
split_leaf(struct Btree_Node *x, uint32_t k, size_t i, struct Btree_Head *head,
      struct list_head *bt)
{
	struct Btree_Node *y = xmalloc(sizeof(*y));
	INIT_LIST_HEAD(&y->u.leaf);
	list_add(&y->u.leaf, &x->u.leaf);
	x->size = BTREE_K;
	y->size = BTREE_K + 1;

	if (i < BTREE_K) {
		memcpy(y->keys, x->keys + BTREE_K - 1, (BTREE_K + 1) * KEY_SZ);
		if (i < BTREE_K - 1)
			memmove(x->keys + i + 1, x->keys + i,
				(BTREE_K - 1) * KEY_SZ);
		x->keys[i] = k;
	} else {
		if (i > BTREE_K)
			memcpy(y->keys, x->keys + BTREE_K,
			       (i - BTREE_K) * KEY_SZ);
		if (i < BTREE_2K)
			memcpy(y->keys + i - BTREE_K + 1, x->keys + i,
			       (BTREE_2K - i) * KEY_SZ);
		y->keys[i - BTREE_K] = k;
	}

	struct Btree_Node *p; /* parent */

	if (list_empty(bt)) {
		p = xmalloc(sizeof(*p));
		*p->keys = max_key(x);
		p->size = 1;
		p->u.sons[0] = x;
		p->u.sons[1] = y;

		head->root = p;
		++head->height;

		assert(head->height = 1);
		return;
	}

	/* XXX The function gets too long! */

	size_t sid;
	p = pop(bt, &sid);

	if (is_full(p)) {
		assert(0 == 1); /* XXX not implemented */
	} else {
		if (sid < p->size) {
			memmove(p->keys + sid + 1, p->keys + sid,
				(p->size - sid) * KEY_SZ);
			memmove(p->u.sons + sid + 2, p->u.sons + sid + 1,
				(p->size - sid) * sizeof(*p->u.sons));
		}
		p->keys[sid] = max_key(x);
		p->u.sons[sid + 1] = y;
		++p->size;
	}
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

	LIST_HEAD(bt); /* backtrace, path from root to the leaf */
	x = target_leaf(head, key, &bt);
	const size_t i = _index(key, x->keys, x->size);

	if (i < x->size && x->keys[i] == key)
		return -1; /* key exists */

	if (is_full(x)) {
		if (squeeze_left(key, x, i, &bt) != 0 &&
		    squeeze_right(key, x, i, &bt) != 0)
			split_leaf(x, key, i, head, &bt);
	} else {
		if (i < x->size)
			memmove(x->keys + i + 1, x->keys + i,
				(x->size - i) * KEY_SZ);
		x->keys[i] = key;
		++x->size;
	}

	return 0;
}

#if 0 /* XXX ======================================================== */
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
#endif /* XXX ======================================================= */
