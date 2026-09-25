#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// BlockFiles are expected to have a 8-byte header
// first 3 bytes are "bl" and then a newline,
// the next 4 bytes (BIG ENDIAN!) are the number of blocks in the file,
// followed by another newline.
// Each block is 1024 bytes, 
// but the last block in the file doesn't have to be 
// (BUT it must be at least 1 byte).
typedef struct {
  char* path;
  uint32_t count;
  FILE* fp;
} BlockFile;

#define BLOCK_MAX_LENGTH (1024)

// When a block is read from a file, it's always treated like it's 1024 bytes.
typedef struct {
  uint32_t number;
  uint8_t data[1024];
} Block;

// Opens a new blockfile- if it doesn't exist, one is created.
// Returns null on failure.
BlockFile* blockfile_open(const char* path);

// Creates a new BlockFile (at "dest") and populates it with blocks
// using the contents of another regular file (at "src_path") as the data.
BlockFile* blockfile_from_file(const char* dest, const char* src_path);

// Closes (and frees) a blockfile.
void blockfile_close(BlockFile* bf);

// Removes all empty blocks, updating bf->count.
// Returns the number of removed blocks on success (>= 0), or -1 on failure.
int blockfile_clean(BlockFile* bf);

// Copies the blocks from one file ("from") 
// and appends them to the end of another ("to").
// Returns the number of blocks appended, or -1 on failure
int blockfile_merge(BlockFile* to, const BlockFile* from);

// Creates a new block (with default number 0), returning null on failure.
// You can create an empty block with block_new(NULL, 0).
// The block is allocated on the heap.
Block* block_new(const uint8_t* data, size_t length);

// Frees a block from memory.
void block_free(Block* bl);

// Appends a block to the end of the blockfile,
// returning truthy on success and falsy on failure.
int block_append(BlockFile* bf, Block* bl);

// Allocates for a block,
// and reads its data from a blockfile at a specific location (number).
// Returns null on failure.
Block* block_read(BlockFile* bf, uint32_t number);

// Overwrites a block at a specific location in a blockfile.
// The location is determined by new_block->number.
// Returns truthy upon success and falsy upon failure.
int block_update(BlockFile* bf, Block* new);

// Overwrites a block with zeroes at a specific location in a blockfile.
// Returns truthy upon success and falsy upon failure.
int block_clear(BlockFile* bf, uint32_t number);

#ifdef BLOCKS_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

// calculating a block's offset in a blockfile from its number
static inline long block_offset(uint32_t number) {
  return 8 + (long)number * BLOCK_MAX_LENGTH;
}

// since strdup isn't actually in the C99 standard
static char* blocks_strdup(const char* s) {
  size_t len = strlen(s) + 1;
  char* copy = malloc(len);
  if (!copy) return NULL;
  memcpy(copy, s, len);
  return copy;
}

BlockFile* blockfile_open(const char* path) {
  if (!path) return NULL;
  
  FILE* fp = fopen(path, "r+b");
  int is_new = 0;
  if (!fp) { // create if doesn't exist yet
    fp = fopen(path, "w+b");
    if (!fp) return NULL;
    is_new = 1;
  }
  
  BlockFile* bf = malloc(sizeof(BlockFile));
  if (!bf) {
    fclose(fp);
    return NULL;
  }
  
  bf->path = blocks_strdup(path);
  if (!bf->path) {
    fclose(fp);
    free(bf);
    return NULL;
  }
  
  bf->fp = fp;
  
  if (is_new) {
    bf->count = 0;
    
    uint8_t header[8] = { 'b', 'l', '\n', 0, 0, 0, 0, '\n' };
    if (fwrite(header, 1, 8, fp) != 8) {
      fclose(fp);
      free(bf->path);
      free(bf);
      return NULL;
    }
    fflush(fp);
  } else {
    uint8_t header[8];

    rewind(fp);
    if (fread(header, 1, 8, fp) != 8 ||
        header[0] != 'b' || header[1] != 'l' ||
        header[2] != '\n' || header[7] != '\n') {
      fclose(fp);
      free(bf->path);
      free(bf);
      return NULL; // invalid header
    }

    // remember, BIG ENDIAN
    bf->count = ((uint32_t)header[3] << 24) | ((uint32_t)header[4] << 16) |
                ((uint32_t)header[5] << 8)  |  (uint32_t)header[6];
  }
  
  return bf;
}

BlockFile* blockfile_from_file(const char* dest, const char* src_path) {
  // TODO
}

void blockfile_close(BlockFile* bf) {
  if (!bf) return;
  if (bf->fp) fclose(bf->fp);
  free(bf->path);
  free(bf);
}

int blockfile_clean(BlockFile* bf) {
  // TODO
}

int blockfile_merge(BlockFile* to, const BlockFile* from) {
  // TODO
}

Block* block_new(const uint8_t* data, size_t length) {
  Block* bl = malloc(sizeof(Block));
  if (!bl) return NULL;
  
  bl->number = 0; // new blocks are initialized with number 0

  if (length == 0 && !data) { // creates empty block
    for (size_t i = 0; i < BLOCK_MAX_LENGTH; i++) bl->data[i] = 0;
    return bl;
  }

  if (!data) { // otherwise, if length != 0 and data is null, we have a problem
    free(bl);
    return NULL;
  }

  // TODO check return value?
  memcpy(bl->data, data, length); // copy data into the block

  // write zeroes to the remaining part of the block
  for (size_t i = length; i < BLOCK_MAX_LENGTH; i++) bl->data[i] = 0;

  return bl;
}

void block_free(Block* bl) {
  if (!bl) return;
  free(bl);
}

// helper for writing the count to the header
static int blockfile_write_count(BlockFile* bf) {
  uint8_t count_bytes[4] = {
    (uint8_t)(bf->count >> 24), (uint8_t)(bf->count >> 16),
    (uint8_t)(bf->count >> 8),  (uint8_t)(bf->count)
  };
  
  if (fseek(bf->fp, 3, SEEK_SET) != 0) return 0;
  
  if (fwrite(count_bytes, 1, 4, bf->fp) != 4) return 0;
  
  fflush(bf->fp);
  return 1;
}

int block_append(BlockFile* bf, Block* bl) {
  if (!bf || !bl || !bf->fp) return 0;

  bl->number = bf->count;

  long offset = block_offset(bl->number);
  if (fseek(bf->fp, offset, SEEK_SET) != 0) return 0;

  if (fwrite(bl->data, 1, BLOCK_MAX_LENGTH, bf->fp) != BLOCK_MAX_LENGTH) {
    return 0;
  }

  bf->count++;
  if (!blockfile_write_count(bf)) {
    bf->count--;
    return 0;
  }

  fflush(bf->fp);
  return 1;
}

Block* block_read(BlockFile* bf, uint32_t number) {
  if (!bf || !bf->fp) return NULL;
  if (number >= bf->count) return NULL;
  
  long offset = block_offset(number);
  if (fseek(bf->fp, offset, SEEK_SET) != 0) return NULL;
  
  Block* bl = malloc(sizeof(Block));
  if (!bl) return NULL;
  
  // in case this is a short final block on disk
  memset(bl->data, 0, BLOCK_MAX_LENGTH);
  
  size_t read_bytes = fread(bl->data, 1, BLOCK_MAX_LENGTH, bf->fp);
  if (read_bytes < BLOCK_MAX_LENGTH && ferror(bf->fp)) {
    free(bl);
    return NULL; // io error
  }
  
  // shouldn't happen with the guard check above, but might as well
  if (read_bytes == 0) {
    free(bl);
    return NULL;
  }
  
  bl->number = number;
  return bl;
}

int block_update(BlockFile* bf, Block* new) {
  if (!bf || !bf->fp || !new) return 0;
  if (new->number >= bf->count) return 0; // block doesn't exist
  
  long offset = block_offset(new->number);
  if (fseek(bf->fp, offset, SEEK_SET) != 0) return 0;
  
  if (fwrite(new->data, 1, BLOCK_MAX_LENGTH, bf->fp) != BLOCK_MAX_LENGTH) {
    return 0;
  }
  
  fflush(bf->fp);
  return 1;
}

int block_clear(BlockFile* bf, uint32_t number) {
  // TODO
}

#endif // BLOCKS_IMPLEMENTATION

#endif // BLOCKS_H
