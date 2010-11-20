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

		return 0;
	}

	if (head->height == 1) {
		struct Btree_Leaf *x = head->root;
		size_t i;
		uint32_t *xs = x->vals;

		for (i = 0; i < x->nvals; ++i) {
			if (xs[i] < key) /* descending order */
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

		/* Splitting, pt. 1: create new leaf */
		assert(x->nvals == BTREE_2K);
		struct Btree_Leaf *y = xmalloc(sizeof(*sib));
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
#endif /*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
