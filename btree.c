#include <string.h>
#include <assert.h>

#include "btree.h"
#include "util.h"

static size_t
_index(uint32_t x, uint32_t arr[], size_t size)
{
	size_t i;
	for (i = 0; i < size; ++i) { /* XXX use binary search */
		if (arr[i] >= x)
			break;
	}
	return i;
}

/* Backtrace element */
struct Frame {
	struct list_head h;

	struct Btree_Node *node; /* Pointer to the node */
	size_t idx; /* Index of the node's son that we descend into */
};

static void
push(struct list_head *bt, struct Btree_Node *node, size_t idx)
{
	struct Frame *new = xmalloc(sizeof(*new));
	INIT_LIST_HEAD(&new->h);
	new->node = node;
	new->idx = idx;

	list_add(&new->h, bt);
}

/*
 * Find the leaf that would hold the given key,
 *
 * This function ignores the fact that the leaf may be full.
 *
 * @bt: backtrace of interior nodes -- the path from the root to the
 *      target leaf, excluding the latter
 */
static struct Btree_Leaf *
target_leaf(const struct Btree_Head *head, uint32_t key, struct list_head *bt)
{
	assert(head->root != NULL);
	assert(list_empty(bt));

	const struct Btree_Node *x = head->root;

	uint8_t h;
	for (h = head->height; h != 0; --h) {
		const size_t i = _index(key, x->keys, x->size);
		push(bt, x, i);
		x = x->sons[i];
	}

	return (struct Btree_Leaf *) x;
}

static struct Btree_Leaf *
left_brother(const struct Btree_Leaf *x, const struct list_head *bt)
{
	if (list_empty(bt))
		return NULL;
	const struct Frame *p = list_first_entry(bt, struct Frame, h);
	return p->idx == 0 ? NULL : p->node->sons[p->idx - 1];
}

static struct Btree_Leaf *
right_brother(const struct Btree_Leaf *x, const struct list_head *bt)
{
	if (list_empty(bt))
		return NULL;
	const struct Frame *p = list_first_entry(bt, struct Frame, h);
	return p->idx == p->node->size ? NULL : p->node->sons[p->idx + 1];
}

int
btree_insert(struct Btree_Head *head, uint32_t key)
{
	LIST_HEAD(bt); /* backtrace */
	struct Btree_Leaf *x = target_leaf(head, key, &bt);
	const size_t i = _index(key, x->vals, x->size);

	if (i < x->size && x->vals[i] == key)
		return -1; /* key exists */

	if (x->size < ARRAY_SIZE(x->vals)) {
		if (i < x->size)
			memmove(x->vals + i + 1, x->vals + i, x->size - i);

		x->vals[i] = key;
		++x->size;

		return 0;
	}

#warning "XXX Check left/right brothers for vacancies."

	struct Btree_Leaf *y = left_brother(x, &bt);

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
	if (head->height == 0)
		return;

	if (head->height > 1) {
		LIST_HEAD(bt); /* backtrace */
		struct Btree_Node *node = head->root;
		struct Btree_Node *son;
		uint8_t depth = 0;

		for (;;) {
			if ((son = node->sons[node->size]) == NULL) {
				assert(node->size == 0);
				if (depth == 0)
					goto end;

				node = pop(&bt);
				--depth;
				continue;
			}

			if (depth == head->height - 2) {
				/* right above leaf level */
				size_t i;
				for (i = 0; i <= node->size; ++i)
					free(node->sons[i]);
				node->size = 0;
				*node->sons = NULL;
			} else {
				if (node->size == 0)
					*node->sons = NULL;
				else
					--node->size;

				push(&bt, node);
				node = son;
				++depth;
			}
		}
	}

end:
	free(head->root);
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
