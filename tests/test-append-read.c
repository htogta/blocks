#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-append-read.blocks"

int main(void) {
  remove(PATH);
  
  BlockFile* bf = blockfile_open(PATH);
  assert(bf != NULL);

  // creating the blocks we're about to append
  Block* a = block_new((const uint8_t*)"first", 5);
  Block* b = block_new((const uint8_t*)"second", 6);
  assert(a != NULL && b != NULL);

  // appending assigns block numbers and increases the count
  int ok = block_append(bf, a);
  assert(ok);
  assert(a->number == 0);
  assert(bf->count == 1);

  ok = block_append(bf, b);
  assert(ok);
  assert(b->number == 1);
  assert(bf->count == 2);

  block_free(a);
  block_free(b);

  // testing a completely full 1024-byte block
  uint8_t full[BLOCK_MAX_LENGTH];
  // just some arbitrary data to fill the block with
  for (int i = 0; i < BLOCK_MAX_LENGTH; i++) full[i] = (uint8_t)(i);
  
  Block* c = block_new(full, BLOCK_MAX_LENGTH);
  assert(c != NULL);
  ok = block_append(bf, c);
  assert(ok);
  block_free(c);
  assert(bf->count == 3);

  // read the blocks back
  Block* r;
  
  r = block_read(bf, 0);
  assert(r != NULL);
  assert(r->number == 0);
  assert(memcmp(r->data, "first", 5) == 0);

  // make sure the rest of the data is zeroes
  for (int i = 5; i < BLOCK_MAX_LENGTH; i++) assert(r->data[i] == 0);
  
  block_free(r);

  // again for the "second" block:
  r = block_read(bf, 1);
  assert(r != NULL);
  assert(r->number == 1);
  assert(memcmp(r->data, "second", 6) == 0);

  // make sure the rest of the data is zeroes
  for (int i = 6; i < BLOCK_MAX_LENGTH; i++) assert(r->data[i] == 0);
  
  block_free(r);
  
  // reading the full block
  r = block_read(bf, 2);
  assert(r != NULL);
  assert(memcmp(r->data, full, BLOCK_MAX_LENGTH) == 0);
  block_free(r);

  // out of bounds reads fail
  Block* none = block_read(bf, 3);
  assert(none == NULL);
  assert(blocks_failure_reason() != NULL);

  // close and reopen and make sure everything's still there
  blockfile_close(bf);
  bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 3);

  r = block_read(bf, 0);
  assert(r != NULL);
  assert(memcmp(r->data, "first", 5) == 0);
  for (int i = 5; i < BLOCK_MAX_LENGTH; i++) assert(r->data[i] == 0);
  block_free(r);

  r = block_read(bf, 1);
  assert(r != NULL);
  assert(r->number == 1);
  assert(memcmp(r->data, "second", 6) == 0);
  for (int i = 6; i < BLOCK_MAX_LENGTH; i++) assert(r->data[i] == 0);
  block_free(r);

  r = block_read(bf, 2);
  assert(r != NULL);
  assert(memcmp(r->data, full, BLOCK_MAX_LENGTH) == 0);
  block_free(r);

  blockfile_close(bf);
  remove(PATH);
  printf("...test-append-read PASSED\n");
  return 0;
}
