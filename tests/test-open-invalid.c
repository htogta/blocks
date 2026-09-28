#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-open-invalid.blocks"

// helper for writing malformed headers
static void write_raw(const uint8_t* bytes, size_t n) {
  FILE* fp = fopen(PATH, "wb");
  assert(fp != NULL);
  size_t written = fwrite(bytes, 1, n, fp);
  assert(written == n);
  fclose(fp);
}

// open PATH, expects failure with set reason
static void expect_open_fails(void) {
  BlockFile* bf = blockfile_open(PATH);
  assert(bf == NULL);
  assert(blocks_failure_reason() != NULL);
}

int main(void) {
  // NULL path
  BlockFile* none = blockfile_open(NULL);
  assert(none == NULL);
  assert(blocks_failure_reason() != NULL);

  // no header
  write_raw((const uint8_t*)"", 0);
  expect_open_fails();

  // header too short
  const uint8_t too_short[] = { 'b', 'l', '\n' };
  write_raw(too_short, sizeof(too_short));
  expect_open_fails();

  // wrong magic number in header
  const uint8_t bad_magic[8] = { 'x', 'l', '\n', 0, 0, 0, 0, '\n' };
  write_raw(bad_magic, 8);
  expect_open_fails();

  // missing trailing newline
  const uint8_t bad_newline[8] = { 'b', 'l', '\n', 0, 0, 0, 0, 'X' };
  write_raw(bad_newline, 8);
  expect_open_fails();
  
  // count in header doesn't agree with the # of blocks present
  const uint8_t liar[8] = { 'b', 'l', '\n', 0, 0, 0, 5, '\n' };
  write_raw(liar, 8);
  expect_open_fails();
  assert(strstr(blocks_failure_reason(), "smaller") != NULL);
  
  // header claims 2 blocks, so the file needs at least 8 + 1024 + 1 bytes-
  // this one is one byte too short
  static uint8_t buf[8 + 1024 + 1];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, "bl\n", 3);
  buf[6] = 2;
  buf[7] = '\n';
  write_raw(buf, 8 + 1024);
  expect_open_fails();

  // whereas this one has the exact right amount
  write_raw(buf, 8 + 1024 + 1);
  BlockFile* bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 2);
  blockfile_close(bf);

  remove(PATH);
  printf("...test-open-invalid PASSED\n");
  return 0;
}
