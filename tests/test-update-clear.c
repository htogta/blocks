#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-update-clear.blocks"

// helper for verifying a block is zeroed
static int block_is_zero(const Block* bl) {
  for (int i = 0; i < BLOCK_MAX_LENGTH; i++) {
    if (bl->data[i] != 0) return 0;
  }
  return 1;
}

int main(void) {
  remove(PATH);

  BlockFile* bf = blockfile_create(PATH, 3);
  assert(bf != NULL);
  
  // fill a few blocks with some data
  const char* contents[3] = { "aaa", "bbb", "ccc" };
  for (int i = 0; i < 3; i++) {
    Block* bl = block_new((const uint8_t*)contents[i], 3);
    assert(bl != NULL);
    bl->number = i;
    int ok = block_update(bf, bl);
    assert(ok);
    block_free(bl);
  }

  // update block 1 in place
  Block* bl = block_read(bf, 1);
  assert(bl != NULL);
  bl->data[0] = 'X';
  int ok = block_update(bf, bl);
  assert(ok);
  block_free(bl);

  // check that the update worked
  bl = block_read(bf, 1);
  assert(bl != NULL);
  assert(memcmp(bl->data, "Xbb", 3) == 0);
  block_free(bl);

  // make sure the neighboring blocks haven't changed
  bl = block_read(bf, 0);
  assert(memcmp(bl->data, "aaa", 3) == 0);
  block_free(bl);
  bl = block_read(bf, 2);
  assert(memcmp(bl->data, "ccc", 3) == 0);
  block_free(bl);

  // updating a block that doesn't exist (outside of capacity) fails
  Block* ghost = block_new((const uint8_t*)"zzz", 3);
  assert(ghost != NULL);
  ghost->number = 3;
  ok = block_update(bf, ghost);
  assert(!ok);
  assert(blocks_failure_reason() != NULL);
  assert(bf->count == 3);
  block_free(ghost);

  // clearing block 2 zeroes it
  ok = block_clear(bf, 2);
  assert(ok);
  assert(bf->count == 3);
  
  bl = block_read(bf, 2);
  assert(bl != NULL);
  assert(block_is_zero(bl));
  block_free(bl);

  // block 0 is untouched
  bl = block_read(bf, 0);
  assert(memcmp(bl->data, "aaa", 3) == 0);
  block_free(bl);

  // block 1 is untouched
  bl = block_read(bf, 1);
  assert(memcmp(bl->data, "Xbb", 3) == 0);
  block_free(bl);

  // clearing out of bounds fails
  ok = block_clear(bf, 3);
  assert(!ok);
  assert(bf->count == 3);

  // changes should persist when we reopen
  blockfile_close(bf);
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 3);

  bl = block_read(bf, 0);
  assert(memcmp(bl->data, "aaa", 3) == 0);
  block_free(bl);
  bl = block_read(bf, 1);
  assert(memcmp(bl->data, "Xbb", 3) == 0);
  block_free(bl);
  bl = block_read(bf, 2);
  assert(block_is_zero(bl));
  block_free(bl);

  blockfile_close(bf);
  remove(PATH);
  printf("...test-update-clear PASSED\n");
  return 0;
}
