#include <string.h>
#include <assert.h>

#include "btree.h"
#include "util.h"

int
btree_insert(struct Btree_Head *head, uint32_t key)
{
	if (head->height == 0) {
		assert(head->root == NULL);

		struct Btree_Leaf *x = xmalloc(sizeof(*x));
		INIT_LIST_HEAD(&x->h);

		*x->vals = key;
		x->nvals = 1;

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
		for (i = 0; i < x->nvals; ++i) {
			if (xs[i] > key)
				break;
			else if (xs[i] == key)
				return -1; /* already present */
		}

		if (x->nvals < BTREE_2K) { /* no need to split */
			if (i != BTREE_2K - 1)
				memmove(xs + i + 1, xs + i,
					(BTREE_2K - i - 1) * sizeof(*xs));
			xs[i] = key;
			++x->nvals;
			return 0;
		}

		/* Splitting, pt. 1: create a sibling leaf */
		assert(x->nvals == BTREE_2K);
		struct Btree_Leaf *y = xmalloc(sizeof(*y));
		INIT_LIST_HEAD(&y->h);
		uint32_t *ys = y->vals;

		x->nvals = BTREE_K + 1;
		y->nvals = BTREE_K;
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
		root->nkeys = 1;
		root->keys[0] = xs[BTREE_K];
		root->sons[0] = x;
		root->sons[1] = y;

		head->root = root;
		++head->height;

		return 0;
	}

	assert(0 == 1); /* XXX not implemented */
}

/* Backtrace element */
struct Frame {
	struct list_head h;
	struct Btree_Node *node;
};

static void
push(struct list_head *bt, struct Btree_Node *node)
{
	struct Frame *new = xmalloc(sizeof(*new));
	INIT_LIST_HEAD(&new->h);
	new->node = node;

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
	if (head->height == 0)
		return;

	if (head->height > 1) {
		LIST_HEAD(bt); /* backtrace */
		struct Btree_Node *node = head->root;
		struct Btree_Node *son;
		uint8_t depth = 0;

		for (;;) {
			if ((son = node->sons[node->nkeys]) == NULL) {
				assert(node->nkeys == 0);
				if (depth == 0)
					goto end;

				node = pop(&bt);
				--depth;
				continue;
			}

			if (depth == head->height - 2) {
				/* right above leaf level */
				size_t i;
				for (i = 0; i <= node->nkeys; ++i)
					free(node->sons[i]);
				node->nkeys = 0;
				*node->sons = NULL;
			} else {
				if (node->nkeys == 0)
					*node->sons = NULL;
				else
					--node->nkeys;

				push(&bt, node);
				node = son;
				++depth;
			}
		}
	}

end:
	free(head->root);
}

#if 0 /*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
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
#endif /*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
