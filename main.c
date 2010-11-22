#include <assert.h>

#include "btree.h"

int main(void)
{
	BTREE_HEAD(t);
	assert(btree_insert(&t, 85) == 0);

	btree_destroy(&t);

	return 0;
}
