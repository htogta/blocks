#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH_A "failure-reason-a.blocks"
#define PATH_B "failure-reason-b.blocks"

int main(void) {
  // nothing has failed yet
  assert(blocks_failure_reason() == NULL);
  
  // the reason names the function that failed
  Block* none = block_read(NULL, 0);
  assert(none == NULL);
  assert(blocks_failure_reason() != NULL);
  assert(strstr(blocks_failure_reason(), "block_read") != NULL);
  
  BlockFile* nobf = blockfile_open(NULL);
  assert(nobf == NULL);
  assert(strstr(blocks_failure_reason(), "blockfile_open") != NULL);
  
  // when a wrapper function fails because something it called failed,
  // the more specific reason should be present
  remove(PATH_A);
  remove(PATH_B);
  BlockFile* to = blockfile_open(PATH_A);
  BlockFile* from = blockfile_open(PATH_B);
  assert(to != NULL && from != NULL);
  
  Block* bl = block_new((const uint8_t*)"data", 4);
  assert(bl != NULL);
  int ok = block_append(from, bl);
  assert(ok);
  block_free(bl);
  
  // lie about the count so blockfile_merge tries to read a block
  // that isn't actually there
  from->count = 99;
  int merged = blockfile_merge(to, from);
  assert(merged == -1);
  assert(strstr(blocks_failure_reason(), "block_read") != NULL);
  from->count = 1;
  
  // blockfile_from_file refusing to overwrite an existing file
  BlockFile* dup = blockfile_from_file(PATH_A, PATH_B);
  assert(dup == NULL);
  assert(strstr(blocks_failure_reason(), "blockfile_from_file") != NULL);
  
  blockfile_close(to);
  blockfile_close(from);
  
  remove(PATH_A);
  remove(PATH_B);
  
  printf("...test-fail-reason PASSED\n");
  return 0;
}
