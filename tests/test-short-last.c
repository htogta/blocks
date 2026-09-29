#define BLOCKS_IMPLEMENTATION
#include "../blocks.h"

#include <assert.h>
#include <string.h>

#define PATH "test-short-last.blocks"

// the format allows the last block on disk to be shorter than 1024 bytes
// (as long as it's at least 1 byte), so let's write a file like that by hand

int main(void) {
  remove(PATH);

  FILE* fp = fopen(PATH, "wb");
  assert(fp != NULL);

  // making the buffers we'll use for our blocks
  const uint8_t header[8] = { 'b', 'l', '\n', 0, 0, 0, 2, '\n' };
  uint8_t first[BLOCK_MAX_LENGTH];
  uint8_t second[10];
  memset(first, 0xAA, sizeof(first));
  memset(second, 0xBB, sizeof(second));

  // writing the header and the buffers
  size_t w = fwrite(header, 1, 8, fp);
  assert(w == 8);
  w = fwrite(first, 1, sizeof(first), fp);
  assert(w == sizeof(first));
  w = fwrite(second, 1, sizeof(second), fp);
  assert(w == sizeof(second));
  fclose(fp);

  // open the file we just made
  BlockFile* bf = blockfile_open(PATH);
  assert(bf != NULL);
  assert(bf->count == 2);

  // read the short block, should be padded with zeroes up to 1024 bytes
  Block* bl = block_read(bf, 1);
  assert(bl != NULL);
  for (int i = 0; i < 10; i++) assert(bl->data[i] == 0xBB);
  for (int i = 10; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);

  // appending after a short block should still work,
  // and shouldn't corrupt the short block
  Block* c = block_new((const uint8_t*)"third", 5);
  assert(c != NULL);
  int ok = block_append(bf, c);
  assert(ok);
  assert(c->number == 2);
  block_free(c); 

  // the short block is resized to a full 1024 bytes, 
  // thanks to POSIX filling in the "hole" with zeroes.
  // I wanna verify that anyway though
  FILE* raw = fopen(PATH, "rb");
  assert(raw != NULL);
  
  int seek_ok = fseek(raw, 0, SEEK_END);
  assert(seek_ok == 0);
  long size = ftell(raw);
  assert(size == 8 + 3 * BLOCK_MAX_LENGTH);

  // end of the short block's real data
  long gap_start = 8 + BLOCK_MAX_LENGTH + 10;
  // start of the newly appended block 
  long gap_end = 8 + 2 * BLOCK_MAX_LENGTH;
  seek_ok = fseek(raw, gap_start, SEEK_SET);
  assert(seek_ok == 0);
 
  long gap_len = gap_end - gap_start;
  uint8_t* gap = malloc((size_t)gap_len);
  assert(gap != NULL);
  size_t read_n = fread(gap, 1, (size_t)gap_len, raw);
  assert(read_n == (size_t)gap_len);
  for (long i = 0; i < gap_len; i++) assert(gap[i] == 0);
  free(gap);
  fclose(raw);

  // and let's read the previously short block  
  bl = block_read(bf, 1);
  assert(bl != NULL);
  for (int i = 0; i < 10; i++) assert(bl->data[i] == 0xBB);
  for (int i = 10; i < BLOCK_MAX_LENGTH; i++) assert(bl->data[i] == 0);
  block_free(bl);

  // check the third block is fine
  bl = block_read(bf, 2);
  assert(bl != NULL);
  assert(memcmp(bl->data, "third", 5) == 0);
  block_free(bl);
  
  blockfile_close(bf);
  remove(PATH);
  printf("...test-short-last PASSED\n");
  return 0;
}
