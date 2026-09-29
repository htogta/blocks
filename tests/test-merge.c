#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH_A "merge-a.blocks"
#define PATH_B "merge-b.blocks"
#define PATH_C "merge-c.blocks"

static void append_str(BlockFile* bf, const char* s) {
  Block* bl = block_new((const uint8_t*)s, strlen(s));
  assert(bl != NULL);
  int ok = block_append(bf, bl);
  assert(ok);
  block_free(bl);
}

// checking that a block with the right content occurs at the right place
static void expect_block(BlockFile* bf, uint32_t n, const char* s) {
  Block* bl = block_read(bf, n);
  assert(bl != NULL);
  assert(bl->number == n);
  assert(memcmp(bl->data, s, strlen(s)) == 0);
  block_free(bl);
}

int main(void) {
  remove(PATH_A);
  remove(PATH_B);
  remove(PATH_C);
  
  BlockFile* a = blockfile_open(PATH_A);
  BlockFile* b = blockfile_open(PATH_B);
  BlockFile* c = blockfile_open(PATH_C); // this one stays empty
  assert(a != NULL && b != NULL && c != NULL);
  
  append_str(a, "a0");
  append_str(a, "a1");
  append_str(b, "b0");
  append_str(b, "b1");
  append_str(b, "b2");
  
  // merging b onto the end of a
  int merged = blockfile_merge(a, b);
  assert(merged == 3);
  assert(a->count == 5);
  expect_block(a, 0, "a0");
  expect_block(a, 1, "a1");
  expect_block(a, 2, "b0");
  expect_block(a, 3, "b1");
  expect_block(a, 4, "b2");
  
  // b is left alone
  assert(b->count == 3);
  expect_block(b, 0, "b0");
  expect_block(b, 2, "b2");
  
  // merging an empty file appends nothing
  merged = blockfile_merge(a, c);
  assert(merged == 0);
  assert(a->count == 5);
  
  // merging a file into itself duplicates its blocks exactly once
  merged = blockfile_merge(b, b);
  assert(merged == 3);
  assert(b->count == 6);
  expect_block(b, 3, "b0");
  expect_block(b, 5, "b2");
  
  // NULL arguments fail
  merged = blockfile_merge(NULL, b);
  assert(merged == -1);
  merged = blockfile_merge(a, NULL);
  assert(merged == -1);
  
  // merged result persists
  blockfile_close(a);
  a = blockfile_open(PATH_A);
  assert(a != NULL);
  assert(a->count == 5);
  expect_block(a, 4, "b2");
  
  blockfile_close(a);
  blockfile_close(b);
  blockfile_close(c);
  remove(PATH_A);
  remove(PATH_B);
  remove(PATH_C);
  
  printf("...test-merge PASSED\n");
  return 0;
}
