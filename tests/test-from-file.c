#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define SRC  "test-from-file.txt"
#define DEST "test-from-file.blocks"

// writes n bytes of a repeating pattern to SRC, pattern gets stored in *out
static void make_src(size_t n, uint8_t* out) {
  for (size_t i = 0; i < n; i++) out[i] = (uint8_t)(i % 256);
  FILE* fp = fopen(SRC, "wb");
  assert(fp != NULL);
  size_t w = fwrite(out, 1, n, fp);
  assert(w == n);
  fclose(fp);
}

int main(void) {
  static uint8_t data[2500]; // 2500 bytes is 3 blocks (1024 + 1024 + 452)
  
  remove(DEST);
  make_src(2500, data);
  
  BlockFile* bf = blockfile_from_file(DEST, SRC);
  assert(bf != NULL);
  assert(bf->count == 3);
  
  // now we go through each block and make sure they match what's in data
  Block* bl = block_read(bf, 0);
  assert(bl != NULL);
  assert(memcmp(bl->data, data, 1024) == 0);
  block_free(bl);
  
  bl = block_read(bf, 1);
  assert(bl != NULL);
  assert(memcmp(bl->data, data + 1024, 1024) == 0);
  block_free(bl);
  
  bl = block_read(bf, 2);
  assert(bl != NULL);
  assert(memcmp(bl->data, data + 2048, 452) == 0);
  for (int i = 452; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);
  blockfile_close(bf);
  
  // it refuses to overwrite a dest that already exists,
  // and leaves the existing file alone ("or else it gets the hose again")
  BlockFile* again = blockfile_from_file(DEST, SRC);
  assert(again == NULL);
  assert(blocks_failure_reason() != NULL);
  
  bf = blockfile_open(DEST);
  assert(bf != NULL);
  assert(bf->count == 3);
  blockfile_close(bf);
  remove(DEST);
  
  // an exact multiple of the block size shouldn't produce a trailing block
  make_src(2048, data);
  bf = blockfile_from_file(DEST, SRC);
  assert(bf != NULL);
  assert(bf->count == 2);
  blockfile_close(bf);
  remove(DEST);
  
  // one byte over spills into a new block
  make_src(1025, data);
  bf = blockfile_from_file(DEST, SRC);
  assert(bf != NULL);
  assert(bf->count == 2);
  bl = block_read(bf, 1);
  assert(bl != NULL);
  assert(bl->data[0] == data[1024]);
  for (int i = 1; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);
  blockfile_close(bf);
  remove(DEST);
  
  // an empty source file gives an empty blockfile
  make_src(0, data);
  bf = blockfile_from_file(DEST, SRC);
  assert(bf != NULL);
  assert(bf->count == 0);
  blockfile_close(bf);
  remove(DEST);
  
  // a missing source file fails, without creating dest
  remove(SRC);
  bf = blockfile_from_file(DEST, SRC);
  assert(bf == NULL);
  assert(blocks_failure_reason() != NULL);
  FILE* fp = fopen(DEST, "rb");
  assert(fp == NULL);
  
  // NULL args
  bf = blockfile_from_file(NULL, SRC);
  assert(bf == NULL);
  bf = blockfile_from_file(DEST, NULL);
  assert(bf == NULL);

  printf("...test-from-file PASSED\n");
  return 0;
}
