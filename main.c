#include <assert.h>

#include "btree.h"

int main(void)
{
	struct Btree_Head t = { NULL, 0 };
	assert(btree_insert(&t, 85) == 0);

	return 0;
}
