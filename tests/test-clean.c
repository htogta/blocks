#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "clean.blocks"

// helper for appending a string as a block to a blockfile
static void append_str(BlockFile* bf, const char* s) {
  // NULL means an empty block
  Block* bl;
  if (s) {
    bl = block_new((const uint8_t*)s, strlen(s));
  } else {
    bl = block_new(NULL, 0);
  }
  
  assert(bl != NULL);
  int ok = block_append(bf, bl);
  assert(ok);
  block_free(bl);
}

int main(void) {
  remove(PATH);
  
  BlockFile* bf = blockfile_open(PATH);
  assert(bf != NULL);
  
  // cleaning an empty blockfile does nothing
  int removed = blockfile_clean(bf);
  assert(removed == 0);
  assert(bf->count == 0);
  
  // A, (empty), B, (empty), (empty), C, (empty)
  append_str(bf, "A");
  append_str(bf, NULL);
  append_str(bf, "B");
  append_str(bf, NULL);
  append_str(bf, NULL);
  append_str(bf, "C");
  append_str(bf, NULL);
  assert(bf->count == 7);

  removed = blockfile_clean(bf);
  assert(removed == 4);
  assert(bf->count == 3);

  // remaining blocks shifted down with order preserved
  const char* expected[3] = { "A", "B", "C" };
  for (uint32_t i = 0; i < 3; i++) {
    Block* bl = block_read(bf, i);
    assert(bl != NULL);
    assert(bl->number == i);
    assert(bl->data[0] == (uint8_t)expected[i][0]);
    assert(bl->data[1] == 0);
    block_free(bl);
  }

  // the block past the new end is unreachable
  Block* none = block_read(bf, 3);
  assert(none == NULL);

  // new count persists
  blockfile_close(bf);
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 3);

  // cleaning again does nothing
  removed = blockfile_clean(bf);
  assert(removed == 0);
  assert(bf->count == 3);

  // testing appending after clean
  append_str(bf, "D");
  assert(bf->count == 4);
  Block* bl = block_read(bf, 3);
  assert(bl != NULL);
  assert(bl->data[0] == 'D');
  block_free(bl);

  blockfile_close(bf);
  remove(PATH);

  // a file made entirely of empty blocks ends up empty
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  append_str(bf, NULL);
  append_str(bf, NULL);
  removed = blockfile_clean(bf);
  assert(removed == 2);
  assert(bf->count == 0);
  blockfile_close(bf);

  remove(PATH);
  printf("...test-clean PASSED\n");
  return 0;
}
