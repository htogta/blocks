#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-count-endian.blocks"

int main(void) {
  remove(PATH);

  BlockFile* bf = blockfile_open(PATH);
  assert(bf != NULL);
  
  // append 300 empty blocks
  Block* empty = block_new(NULL, 0);
  assert(empty != NULL);
  for (int i = 0; i < 300; i++) {
    int ok = block_append(bf, empty);
    assert(ok);
  }
  block_free(empty);

  // make sure count gets updated
  assert(bf->count == 300);
  blockfile_close(bf);

  // read the header
  FILE* fp = fopen(PATH, "rb");
  assert(fp != NULL);
  uint8_t header[8];
  size_t n = fread(header, 1, 8, fp);
  assert(n == 8);

  // check the file size
  int seek_ok = fseek(fp, 0, SEEK_END);
  assert(seek_ok == 0);
  long size = ftell(fp);
  fclose(fp);
  assert(size == 8 + 300 * BLOCK_MAX_LENGTH);

  // check the header (BIG-ENDIAN)
  assert(header[0] == 'b');
  assert(header[1] == 'l');
  assert(header[2] == '\n');
  assert(header[3] == 0x00);
  assert(header[4] == 0x00);
  assert(header[5] == 0x01);
  assert(header[6] == 0x2C);
  assert(header[7] == '\n');

  // reading it back should give the same count
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 300);
  blockfile_close(bf);

  remove(PATH);
  printf("...test-count-endian PASSED\n");
  return 0;
}
