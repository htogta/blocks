#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-insert.blocks"

static void append_str(BlockFile* bf, const char* s) {
  Block* bl = block_new((const uint8_t*)s, strlen(s));
  assert(bl != NULL);
  int ok = block_append(bf, bl);
  assert(ok);
  block_free(bl);
}

static void expect(BlockFile* bf, uint32_t n, const char* s) {
  Block* bl = block_read(bf, n);
  assert(bl != NULL);
  assert(bl->number == n);
  assert(memcmp(bl->data, s, strlen(s)) == 0);
  block_free(bl);
}

int main(void) {
  remove(PATH);
  
  BlockFile* bf = blockfile_open(PATH);
  assert(bf != NULL);
  
  // A B C D
  append_str(bf, "A"); // 0
  append_str(bf, "B"); // 1
  append_str(bf, "C"); // 2
  append_str(bf, "D"); // 3
  assert(bf->count == 4);
  
  // insert in the middle: A B X C D
  Block* x = block_new((const uint8_t*)"X", 1);
  assert(x != NULL);
  x->number = 2;
  int ok = block_insert(bf, x);
  assert(ok);
  block_free(x);
  assert(bf->count == 5);
  
  expect(bf, 0, "A");
  expect(bf, 1, "B");
  expect(bf, 2, "X");
  expect(bf, 3, "C");
  expect(bf, 4, "D");
  
  // insert at the very front
  Block* y = block_new((const uint8_t*)"Y", 1);
  assert(y != NULL);
  y->number = 0;
  ok = block_insert(bf, y);
  assert(ok);
  block_free(y);
  assert(bf->count == 6);
  
  expect(bf, 0, "Y");
  expect(bf, 1, "A");
  expect(bf, 2, "B");
  expect(bf, 3, "X");
  expect(bf, 4, "C");
  expect(bf, 5, "D");
  
  // inserting at position == count behaves like append
  Block* z = block_new((const uint8_t*)"Z", 1);
  assert(z != NULL);
  z->number = bf->count; // 6
  ok = block_insert(bf, z);
  assert(ok);
  block_free(z);
  assert(bf->count == 7);
  expect(bf, 6, "Z");
  
  // out-of-range insert fails, and doesn't change the file
  Block* bad = block_new((const uint8_t*)"!", 1);
  assert(bad != NULL);
  bad->number = 999;
  ok = block_insert(bf, bad);
  assert(!ok);
  assert(blocks_failure_reason() != NULL);
  assert(bf->count == 7);
  block_free(bad);
  
  // NULL arguments fail cleanly
  ok = block_insert(NULL, bad);
  assert(!ok);
  Block* placeholder = block_new(NULL, 0);
  assert(placeholder != NULL);
  ok = block_insert(bf, NULL);
  assert(!ok);
  block_free(placeholder);
  
  // everything survives closing and reopening
  blockfile_close(bf);
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 7);
  expect(bf, 0, "Y");
  expect(bf, 2, "B");
  expect(bf, 3, "X");
  expect(bf, 6, "Z");
  blockfile_close(bf);
  remove(PATH);
  
  // inserting into an empty blockfile (count == 0) is just the append case
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 0);
  
  Block* first = block_new((const uint8_t*)"first", 5);
  assert(first != NULL);
  first->number = 0;
  ok = block_insert(bf, first);
  assert(ok);
  block_free(first);
  assert(bf->count == 1);
  expect(bf, 0, "first");
  
  blockfile_close(bf);
  remove(PATH);
  
  // inserting before a short last block on disk should shift
  // it into a full, zero-padded 1024-byte block 
  remove(PATH);
  FILE* fp = fopen(PATH, "wb");
  assert(fp != NULL);
  const uint8_t header[8] = { 'b', 'l', '\n', 0, 0, 0, 2, '\n' };
  uint8_t block0[BLOCK_MAX_LENGTH];
  uint8_t short_block[10];
  memset(block0, 0xAA, sizeof(block0));
  memset(short_block, 0xBB, sizeof(short_block));
  size_t w = fwrite(header, 1, 8, fp);
  assert(w == 8);
  w = fwrite(block0, 1, sizeof(block0), fp);
  assert(w == sizeof(block0));
  w = fwrite(short_block, 1, sizeof(short_block), fp);
  assert(w == sizeof(short_block));
  fclose(fp);
  
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 2);
  
  Block* mid = block_new((const uint8_t*)"MID", 3);
  assert(mid != NULL);
  mid->number = 1; // insert before the short last block
  ok = block_insert(bf, mid);
  assert(ok);
  block_free(mid);
  assert(bf->count == 3);
  
  expect(bf, 1, "MID");
  
  Block* shifted = block_read(bf, 2);
  assert(shifted != NULL);
  for (int i = 0; i < 10; i++) assert(shifted->data[i] == 0xBB);
  for (int i = 10; i < BLOCK_MAX_LENGTH; i++) assert(shifted->data[i] == 0);
  block_free(shifted);
  
  // the file itself should now be a clean 3 full blocks,
  // the shifted block is no longer physically short on disk
  FILE* raw = fopen(PATH, "rb");
  assert(raw != NULL);
  int seek_ok = fseek(raw, 0, SEEK_END);
  assert(seek_ok == 0);
  long size = ftell(raw);
  fclose(raw);
  assert(size == 8 + 3 * BLOCK_MAX_LENGTH);
  
  blockfile_close(bf);
  remove(PATH);
  
  // inserting into the middle of a "full" BlockFile should fail. 
  // this targets block_insert's own overflow check specifically, 
  // the append-case branch (number == count) just uses block_append, 
  // which has its own separate overflow guard tested elsewhere
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  // count == 1, so this is a shift-case insert
  append_str(bf, "only-real-block"); 
  
  uint32_t real_count = bf->count;
  // simulate being at the max without writing 4B blocks
  bf->count = UINT32_MAX; 
  
  Block* overflow_block = block_new((const uint8_t*)"nope", 4);
  assert(overflow_block != NULL);
  overflow_block->number = 0; // 0 < UINT32_MAX, so this takes the shift path
  ok = block_insert(bf, overflow_block);
  assert(!ok);
  
  // check specifically for the overflow guard firing
  assert(strstr(blocks_failure_reason(), "overflow") != NULL);
  block_free(overflow_block);
  
  bf->count = real_count; // restore before closing
  blockfile_close(bf);
  remove(PATH);
  
  printf("...test-insert PASSED\n");
  return 0;
}
