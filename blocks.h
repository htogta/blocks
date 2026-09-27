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
  uint8_t data[BLOCK_MAX_LENGTH];
} Block;

// Returns a human-readable string describing the last failure,
// from any of the following API functions. Note that it isn't thread-safe.
const char* blocks_failure_reason(void);

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
// I would be careful using this function, 
// as it won't preserve the relative positions of blocks.
// Think of it like defragmenting a hard drive.

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
Block* block_read(const BlockFile* bf, uint32_t number);

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

// most recent error stored here
static const char* blocks_last_error = NULL;

const char* blocks_failure_reason(void) {
  return blocks_last_error;
}

// records a reason, returns 0 - for int-returning functions
static int blocks_fail(const char* reason) {
  blocks_last_error = reason;
  return 0;
}

// records a reason, returns NULL - for pointer-returning functions
static void* blocks_fail_ptr(const char* reason) {
  blocks_last_error = reason;
  return NULL;
}

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

BlockFile* blockfile_open(const char* path) {
  if (!path) return blocks_fail_ptr("blockfile_open: path is NULL");
  
  FILE* fp = fopen(path, "r+b");
  int is_new = 0;
  if (!fp) { // create if doesn't exist yet
    fp = fopen(path, "w+b");
    
    if (!fp) {
      return blocks_fail_ptr("blockfile_open: failed to create new file");
    }
     
    is_new = 1;
  }
  
  BlockFile* bf = malloc(sizeof(BlockFile));
  if (!bf) {
    fclose(fp);
    return blocks_fail_ptr(
      "blockfile_open: failed to allocate memory for BlockFile");
  }
  
  bf->path = blocks_strdup(path);
  if (!bf->path) {
    fclose(fp);
    free(bf);
    return blocks_fail_ptr(
      "blockfile_open: failed to allocate memory for path");
  }
  
  bf->fp = fp;
  
  if (is_new) {
    bf->count = 0;
    
    uint8_t header[8] = { 'b', 'l', '\n', 0, 0, 0, 0, '\n' };
    if (fwrite(header, 1, 8, fp) != 8) {
      fclose(fp);
      free(bf->path);
      free(bf);
      return blocks_fail_ptr("blockfile_open: failed to write header");
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
      return blocks_fail_ptr("blockfile_open: malformed header");
    }

    // remember, BIG ENDIAN
    bf->count = ((uint32_t)header[3] << 24) | ((uint32_t)header[4] << 16) |
                ((uint32_t)header[5] << 8)  |  (uint32_t)header[6];

    // confirm the file is actually big enough to hold that many blocks
    if (fseek(fp, 0, SEEK_END) != 0) {
      fclose(fp);
      free(bf->path);
      free(bf);
      return blocks_fail_ptr(
        "blockfile_open: failed to seek to end of BlockFile");
    }

    long file_size = ftell(fp);
    if (file_size < 0) {
      fclose(fp);
      free(bf->path);
      free(bf);
      return blocks_fail_ptr(
        "blockfile_open: failed to determine BlockFile size");
    }

    if (bf->count > 0) {
      // every block but the last may be full, the last needs at least 1 byte
      long min_size = 8 + (long)(bf->count - 1) * BLOCK_MAX_LENGTH + 1;
      if (file_size < min_size) {
        fclose(fp);
        free(bf->path);
        free(bf);
        return blocks_fail_ptr(
          "blockfile_open: file is smaller than header claims");
      }
    }
  }
  
  return bf;
}

BlockFile* blockfile_from_file(const char* dest, const char* src_path) {
  if (!dest || !src_path) return NULL;

  FILE* existing = fopen(dest, "rb");
  if (existing) {
    // dest already exists, so don't touch it
    fclose(existing);
    return blocks_fail_ptr("blockfile_from_file: dest file already exists");
  }

  FILE* src = fopen(src_path, "rb");
  if (!src) {
    return blocks_fail_ptr("blockfile_from_file: failed to open src_path");
  }

  // NOTE: there's potential for a race condition here if, between running the
  // above code and the code immediately below this, the file at "dest" is 
  // created. I don't think that's a scenario I'd run into with my typical use
  // of this library, but it's worth mentioning.

  BlockFile* bf = blockfile_open(dest);
  if (!bf) {
    fclose(src);
    return NULL; // blockfile_open already set a specific reason
  }

  uint8_t buffer[BLOCK_MAX_LENGTH];
  size_t read_bytes;
  while ((read_bytes = fread(buffer, 1, BLOCK_MAX_LENGTH, src)) > 0) {
    Block* bl = block_new(buffer, read_bytes);
    if (!bl) {
      fclose(src);
      blockfile_close(bf);
      return NULL; // reason set by block_new
    }

    if (!block_append(bf, bl)) {
      block_free(bl);
      fclose(src);
      blockfile_close(bf);
      return NULL; // reason set by block_append
    }

    block_free(bl);
  }

  if (ferror(src)) {
    fclose(src);
    blockfile_close(bf);
    return blocks_fail_ptr(
      "blockfile_from_file: I/O error reading file at src_path");
  }

  fclose(src);
  return bf;
}

void blockfile_close(BlockFile* bf) {
  if (!bf) {
    blocks_fail("blockfile_close: BlockFile is NULL");
    return;
  }
  
  if (bf->fp) fclose(bf->fp);
  free(bf->path);
  free(bf);
}

// helper for checking that a block is empty
static int block_is_empty(const Block* bl) {
  for (size_t i = 0; i < BLOCK_MAX_LENGTH; i++) {
    if (bl->data[i] != 0) return 0;
  }
  return 1;
}

int blockfile_clean(BlockFile* bf) {
  if (!bf) {
    blocks_fail("blockfile_clean: BlockFile is NULL");
    return -1;
  }
  
  if (!bf->fp) {
    blocks_fail("blockfile_clean: BlockFile file pointer is NULL");
    return -1;
  } 

  uint32_t write_index = 0;
  int removed = 0;

  for (uint32_t read_index = 0; read_index < bf->count; read_index++) {
    Block* bl = block_read(bf, read_index);
    if (!bl) return -1; // reason set by block_read

    if (block_is_empty(bl)) {
      removed++;
      block_free(bl);
      continue;
    }

    if (write_index != read_index) {
      // shift this block down to close the gap
      long offset = block_offset(write_index);
      if (fseek(bf->fp, offset, SEEK_SET) != 0 ||
          fwrite(bl->data, 1, BLOCK_MAX_LENGTH, bf->fp) != BLOCK_MAX_LENGTH) {
        block_free(bl);
        blocks_fail(
          "blockfile_clean: failed to shift Block after clearing previous Block"
        );
        return -1;
      }
    }

    write_index++;
    block_free(bl);
  }

  fflush(bf->fp);

  bf->count = write_index;
  if (!blockfile_write_count(bf)) {
    blocks_fail("blockfile_clean: failed to write count to BlockFile header");
    return -1;
  } 

  return removed;
}

int blockfile_merge(BlockFile* to, const BlockFile* from) {
  if (!to) {
    blocks_fail("blockfile_merge: destination BlockFile is NULL");
    return -1;
  }

  if (!from) {
    blocks_fail("blockfile_merge: source BlockFile is NULL");
    return -1;
  }

  if (!to->fp) {
    blocks_fail("blockfile_merge: destination BlockFile's file pointer is NULL");
    return -1;
  }

  if (!from->fp) {
    blocks_fail(
      "blockfile_merge: source BlockFile's file pointer is NULL");
    return -1;
  }

  uint32_t original_count = from->count;
  uint32_t appended = 0;

  for (uint32_t i = 0; i < original_count; i++) {
    Block* bl = block_read(from, i);
    if (!bl) return -1; // reason set by block_read

    int ok = block_append(to, bl);
    block_free(bl);

    if (!ok) return -1; // reason set by block_append
    
    appended++;
  }

  return (int)appended;
}

Block* block_new(const uint8_t* data, size_t length) {
  if (length > BLOCK_MAX_LENGTH) {
    return blocks_fail_ptr("block_new: length > BLOCK_MAX_LENGTH");
  }

  Block* bl = malloc(sizeof(Block));
  if (!bl) {
    return blocks_fail_ptr("block_new: failed to allocate memory for Block");
  }
  
  bl->number = 0; // new blocks are initialized with number 0

  if (length == 0 && !data) { // creates empty block
    memset(bl->data, 0, BLOCK_MAX_LENGTH);
    return bl;
  }

  if (!data) { // otherwise, if length != 0 and data is null, we have a problem
    free(bl);
    return blocks_fail_ptr(
      "block_new: Block length is nonzero but data is NULL");
  }
  
  memcpy(bl->data, data, length); // copy data into the block
  memset(bl->data + length, 0, BLOCK_MAX_LENGTH - length); // zero what's left

  return bl;
}

void block_free(Block* bl) {
  if (!bl) return;
  free(bl);
}

int block_append(BlockFile* bf, Block* bl) {
  if (!bf) return blocks_fail("block_append: BlockFile is NULL");
  if (!bl) return blocks_fail("block_append: Block is NULL");
  
  if (!bf->fp) {
    return blocks_fail("block_append: BlockFile's file pointer is NULL");
  } 
  
  if (bf->count == UINT32_MAX) {
    return blocks_fail("block_append: BlockFile count overflow");
  } 

  bl->number = bf->count;

  long offset = block_offset(bl->number);
  if (fseek(bf->fp, offset, SEEK_SET) != 0) {
    return blocks_fail("block_append: failed to seek to end of BlockFile");
  }

  if (fwrite(bl->data, 1, BLOCK_MAX_LENGTH, bf->fp) != BLOCK_MAX_LENGTH) {
    return blocks_fail("block_append: failed to write new Block to BlockFile");
  }

  bf->count++;
  if (!blockfile_write_count(bf)) {
    bf->count--;
    return blocks_fail(
      "block_append: failed to write count to BlockFile header");
  }

  fflush(bf->fp);
  return 1;
}

Block* block_read(const BlockFile* bf, uint32_t number) {
  if (!bf) return blocks_fail_ptr("block_read: BlockFile is NULL");
  
  if (!bf->fp) {
    return blocks_fail_ptr("block_read: BlockFile file pointer is NULL");
  }

  if (number >= bf->count) {
    return blocks_fail_ptr("block_read: Block number is out of bounds");
  } 
  
  long offset = block_offset(number);
  if (fseek(bf->fp, offset, SEEK_SET) != 0) {
    return blocks_fail_ptr(
      "block_read: failed to seek to Block position in BlockFile");
  }
  
  Block* bl = malloc(sizeof(Block));
  if (!bl) return blocks_fail_ptr("block_read: failed to allocate Block");
  
  // in case this is a short final block on disk
  memset(bl->data, 0, BLOCK_MAX_LENGTH);
  
  size_t read_bytes = fread(bl->data, 1, BLOCK_MAX_LENGTH, bf->fp);
  if (read_bytes < BLOCK_MAX_LENGTH && ferror(bf->fp)) {
    free(bl);
    return blocks_fail_ptr("block_read: I/O error when reading Block");
  }
  
  // shouldn't happen with the guard above, but might as well
  if (read_bytes == 0) {
    free(bl);
    return blocks_fail_ptr("block_read: I/O error when reading Block");
  }
  
  bl->number = number;
  return bl;
}

int block_update(BlockFile* bf, Block* new_block) {
  if (!bf) return blocks_fail("block_update: BlockFile is NULL");
  
  if (!bf->fp) { 
    return blocks_fail("block_update: BlockFile file pointer is NULL");
  }
  
  if (!new_block) return blocks_fail("block_update: new Block is NULL");
  
  if (new_block->number >= bf->count) {
    return blocks_fail(
      "block_update: Block doesn't exist in BlockFile (out of bounds)");
  }
  
  long offset = block_offset(new_block->number);
  if (fseek(bf->fp, offset, SEEK_SET) != 0) {
    return blocks_fail(
      "block_update: failed to seek to Block position in BlockFile"
    );
  }
  
  if (
    fwrite(new_block->data, 1, BLOCK_MAX_LENGTH, bf->fp) != BLOCK_MAX_LENGTH
  ) return blocks_fail("block_update: failed to write Block data to BlockFile");
  
  fflush(bf->fp);
  return 1;
}

int block_clear(BlockFile* bf, uint32_t number) {
  if (!bf) return blocks_fail("block_clear: BlockFile is NULL");

  if (number >= bf->count) {
    return blocks_fail("block_clear: Block number is out of bounds");
  }

  Block* empty_block = block_new(NULL, 0);
  if (!empty_block) return 0; // reason set by block_new

  empty_block->number = number;
  int ok = block_update(bf, empty_block);
  block_free(empty_block);

  return ok; // reason set by block_update
}

#endif // BLOCKS_IMPLEMENTATION

#endif // BLOCKS_H
