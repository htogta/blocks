#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

int main(void) {
  // empty block: number 0, all zeroes
  Block* bl = block_new(NULL, 0);
  assert(bl != NULL);
  assert(bl->number == 0);
  for (int i = 0; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);

  // partial data gets padded with zeroes
  bl = block_new((const uint8_t*)"hello", 5);
  assert(bl != NULL);
  assert(bl->number == 0);
  assert(memcmp(bl->data, "hello", 5) == 0);
  for (int i = 5; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);

  // full block
  uint8_t full[BLOCK_MAX_LENGTH];
  memset(full, 0xAB, sizeof(full));
  bl = block_new(full, BLOCK_MAX_LENGTH);
  assert(bl != NULL);
  assert(memcmp(bl->data, full, BLOCK_MAX_LENGTH) == 0);
  block_free(bl);

  // one byte too much
  uint8_t too_big[BLOCK_MAX_LENGTH + 1];
  memset(too_big, 0xAB, sizeof(too_big));
  bl = block_new(too_big, BLOCK_MAX_LENGTH + 1);
  assert(bl == NULL);
  assert(blocks_failure_reason() != NULL);

  // NULL data with a nonzero length
  bl = block_new(NULL, 5);
  assert(bl == NULL);
  assert(blocks_failure_reason() != NULL);

  // freeing NULL is fine (TODO maybe that's not a good thing)
  block_free(NULL);

  printf("...test-block-new PASSED\n");
  return 0;
}
