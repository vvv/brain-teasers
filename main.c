#include <assert.h>

#include "btree.h"

int main(void)
{
	BTREE_HEAD(t);
	assert(btree_insert(&t, 177) == 0);
	assert(btree_insert(&t, 248) == 0);
	assert(btree_insert(&t, 124) == 0);
	assert(btree_insert(&t, 75) == 0);
	assert(btree_insert(&t, 133) == 0);
	assert(btree_insert(&t, 230) == 0);
	assert(btree_insert(&t, 129) == 0);
	assert(btree_insert(&t, 101) == 0);
	assert(btree_insert(&t, 140) == 0);
	assert(btree_insert(&t, 143) == 0);
	assert(btree_insert(&t, 246) == 0);
	assert(btree_insert(&t, 113) == 0);
	assert(btree_insert(&t, 167) == 0);
	assert(btree_insert(&t, 142) == 0);
	assert(btree_insert(&t, 72) == 0);
	assert(btree_insert(&t, 227) == 0);
	assert(btree_insert(&t, 235) == 0);
	assert(btree_insert(&t, 103) == 0);
	assert(btree_insert(&t, 103) == -1);
	assert(btree_insert(&t, 28) == 0);
	assert(btree_insert(&t, 113) == -1);
	assert(btree_insert(&t, 250) == 0);
	assert(btree_insert(&t, 44) == 0);
	assert(btree_insert(&t, 35) == 0);
	assert(btree_insert(&t, 130) == 0);
	assert(btree_insert(&t, 9) == 0);
	assert(btree_insert(&t, 196) == 0);
	assert(btree_insert(&t, 40) == 0);
	assert(btree_insert(&t, 125) == 0);
	assert(btree_insert(&t, 34) == 0);
	assert(btree_insert(&t, 245) == 0);
	assert(btree_insert(&t, 102) == 0);
	assert(btree_insert(&t, 245) == 0);
	assert(btree_insert(&t, 40) == 0);
	assert(btree_insert(&t, 216) == 0);
	assert(btree_insert(&t, 213) == 0);
	assert(btree_insert(&t, 57) == 0);
	assert(btree_insert(&t, 37) == 0);
	assert(btree_insert(&t, 245) == 0);
	assert(btree_insert(&t, 96) == 0);
	assert(btree_insert(&t, 7) == 0);
	assert(btree_insert(&t, 130) == 0);
	assert(btree_insert(&t, 78) == 0);
	assert(btree_insert(&t, 146) == 0);
	assert(btree_insert(&t, 247) == 0);
	assert(btree_insert(&t, 32) == 0);
	assert(btree_insert(&t, 251) == 0);
	assert(btree_insert(&t, 253) == 0);
	assert(btree_insert(&t, 113) == 0);
	assert(btree_insert(&t, 36) == 0);
	assert(btree_insert(&t, 62) == 0);
	assert(btree_insert(&t, 28) == 0);
	assert(btree_insert(&t, 31) == 0);
	assert(btree_insert(&t, 183) == 0);
	assert(btree_insert(&t, 53) == 0);
	assert(btree_insert(&t, 118) == 0);
	assert(btree_insert(&t, 80) == 0);
	assert(btree_insert(&t, 163) == 0);
	assert(btree_insert(&t, 243) == 0);
	assert(btree_insert(&t, 183) == 0);
	assert(btree_insert(&t, 141) == 0);
	assert(btree_insert(&t, 222) == 0);
	assert(btree_insert(&t, 49) == 0);
	assert(btree_insert(&t, 252) == 0);
	assert(btree_insert(&t, 221) == 0);
	assert(btree_insert(&t, 179) == 0);
	assert(btree_insert(&t, 245) == 0);
	assert(btree_insert(&t, 48) == 0);
	assert(btree_insert(&t, 251) == 0);
	assert(btree_insert(&t, 74) == 0);
	assert(btree_insert(&t, 228) == 0);
	assert(btree_insert(&t, 199) == 0);
	assert(btree_insert(&t, 9) == 0);
	assert(btree_insert(&t, 121) == 0);
	assert(btree_insert(&t, 17) == 0);
	assert(btree_insert(&t, 133) == 0);
	assert(btree_insert(&t, 255) == 0);
	assert(btree_insert(&t, 166) == 0);
	assert(btree_insert(&t, 169) == 0);
	assert(btree_insert(&t, 163) == 0);
	assert(btree_insert(&t, 225) == 0);
	assert(btree_insert(&t, 213) == 0);
	assert(btree_insert(&t, 39) == 0);
	assert(btree_insert(&t, 31) == 0);
	assert(btree_insert(&t, 37) == 0);
	assert(btree_insert(&t, 162) == 0);
	assert(btree_insert(&t, 7) == 0);
	assert(btree_insert(&t, 195) == 0);
	assert(btree_insert(&t, 199) == 0);
	assert(btree_insert(&t, 160) == 0);
	assert(btree_insert(&t, 164) == 0);
	assert(btree_insert(&t, 246) == 0);
	assert(btree_insert(&t, 224) == 0);
	assert(btree_insert(&t, 19) == 0);
	assert(btree_insert(&t, 171) == 0);
	assert(btree_insert(&t, 186) == 0);
	assert(btree_insert(&t, 81) == 0);
	assert(btree_insert(&t, 45) == 0);
	assert(btree_insert(&t, 197) == 0);
	assert(btree_insert(&t, 206) == 0);

	btree_destroy(&t);

	return 0;
}
