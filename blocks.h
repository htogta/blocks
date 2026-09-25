#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdint.h>
#include <stddef.h>

// BlockFiles are expected to have a 8-byte header
// first 3 bytes are "bl" and then a newline,
// the next 4 bytes (BIG ENDIAN!) are the number of blocks in the file,
// followed by another newline.
// Each block is 1024 bytes, 
// but the last block in the file doesn't have to be 
// (BUT it must be at least 1 byte).
typedef struct {
  const char* path;
  uint32_t count;
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
int block_update(BlockFile* bf, Block* new_block);

// Overwrites a block with zeroes at a specific location in a blockfile.
// Returns truthy upon success and falsy upon failure.
int block_clear(BlockFile* bf, uint32_t number);

#ifdef BLOCKS_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

BlockFile* blockfile_open(const char* path) {
  // TODO
}

BlockFile* blockfile_from_file(const char* dest, const char* src_path) {
  // TODO
}

void blockfile_close(BlockFile* bf) {
  // TODO
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
  free(bl);
}

int block_append(BlockFile* bf, Block* bl) {
  // TODO
}

Block* block_read(BlockFile* bf, uint32_t number) {
  // TODO
}

int block_update(BlockFile* bf, Block* new_block) {
  // TODO
}

int block_clear(BlockFile* bf, uint32_t number) {
  // TODO
}

#endif // BLOCKS_IMPLEMENTATION

#endif // BLOCKS_H
