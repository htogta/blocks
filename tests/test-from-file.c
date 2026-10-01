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

  // first make a blockfile big enough to fit src
  BlockFile* bf = blockfile_create(DEST, 3);
  assert(bf != NULL);
  assert(bf->count == 3);

  // now load the file
  int ok = blockfile_load_file(bf, 0, SRC);
  assert(ok);
  
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

  // now make sure we can't overwrite the data
  ok = blockfile_load_file(bf, 0, SRC); // should fail, nonzero'd blocks
  assert(!ok);

  // check that we can't load the data if there's not enough remaining blocks
  ok = blockfile_load_file(bf, 1, SRC);
  assert(!ok);
  blockfile_close(bf);
  remove(DEST);
  
  // create another file that doesn't have enough space
  bf = blockfile_create(DEST, 2);
  assert(bf != NULL); 
  assert(bf->count == 2);
  ok = blockfile_load_file(bf, 0, SRC);
  assert(!ok);
  
  // an exact multiple of the block size shouldn't produce a trailing block
  // so this shouldn't fail
  remove(SRC);
  make_src(2048, data);
  ok = blockfile_load_file(bf, 0, SRC);
  assert(ok);
  
  blockfile_close(bf);
  remove(DEST);
  
  // one byte over spills into a new block
  bf = blockfile_create(DEST, 2);
  assert(bf != NULL);
  
  make_src(1025, data);
  ok = blockfile_load_file(bf, 0, SRC);
  assert(ok);
  
  bl = block_read(bf, 1);
  assert(bl != NULL);
  assert(bl->data[0] == data[1024]);
  for (int i = 1; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);
  blockfile_close(bf);
  remove(DEST);
  
  // an empty source file gives an empty blockfile
  bf = blockfile_create(DEST, 2);
  make_src(0, data);
  ok = blockfile_load_file(bf, 0, SRC);
  assert(ok);

  bl = block_read(bf, 0);
  for (int i = 0; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);
  
  blockfile_close(bf);
  remove(DEST);
  
  // a missing source file fails, without creating dest
  remove(SRC);

  bf = blockfile_create(DEST, 2);
  assert(bf != NULL);
  ok = blockfile_load_file(bf, 0, SRC);
  assert(!ok);
  assert(blocks_failure_reason() != NULL);
  
  // NULL args
  ok = blockfile_load_file(NULL, 0, SRC);
  assert(!ok);
  ok = blockfile_load_file(bf, 0, NULL);
  assert(!ok);

  blockfile_close(bf);
  remove(DEST);

  printf("...test-from-file PASSED\n");
  return 0;
}
