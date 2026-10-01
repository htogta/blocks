#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-open-new.blocks"

int main(void) {
  remove(PATH);

  // opening a file that doesn't exist should fail
  BlockFile* bf = blockfile_open(PATH);
  assert(bf == NULL);
  assert(blocks_failure_reason() != NULL);
  assert(strstr(blocks_failure_reason(), "blockfile_open") != NULL);

  // creating a new file should work tho
  bf = blockfile_create(PATH, 50);
  assert(bf != NULL);
  assert(bf->count == 50);
  assert(strcmp(bf->path, PATH) == 0);
  blockfile_close(bf);

  // checking the header
  FILE* fp = fopen(PATH, "rb");
  assert(fp != NULL);
  uint8_t buf[8];
  size_t n = fread(buf, 1, sizeof(buf), fp);
  fclose(fp);
  
  const uint8_t expected[8] = { 'b', 'l', '\n', 0, 0, 0, 50, '\n' };
  assert(n == 8);
  assert(memcmp(buf, expected, 8) == 0);

  // reopening should work & blockfile should still be empty
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 50);
  blockfile_close(bf);

  remove(PATH);
  printf("...test-open-new PASSED\n");
  return 0;
}
