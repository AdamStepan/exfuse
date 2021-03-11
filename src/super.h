/**
 * @file super.h
 *
 * This file defines the super block, block bitmaps and their API.
 */
#ifndef EX_SUPER_H
#define EX_SUPER_H

#include "errors.h"

#include <stdint.h>
#include <sys/statvfs.h>

/** This defines block size. */
#define EX_BLOCK_SIZE 4096
/** Maximum filename basename length. */
#define EX_NAME_LEN 54
/** Super block magic number */
#define EX_SUPER_MAGIC 0xffaacc

/** @deprecated functions should return ex_status instead of arbitraty return code. */
#define EX_BLOCK_INVALID_ID ((size_t)-1)

/** @deprecated functions should return ex_status instead of arbitraty return code. */
#define EX_BLOCK_INVALID_ADDRESS ((size_t)-1)

typedef size_t inode_address;
typedef size_t block_address;

/** Runtime representation of an inode block. */
struct ex_inode_block {
    /** Position in super block inodes block bitmap. */
    size_t id;
    /** Address of inode block. */
    size_t address;
    /** Blocks data.
     *
     * They're not loaded by default
     */
    char *data;
};

/** The block allocation bitmap
 *
 * It's used for allocation of inode and data blocks.
 */
struct ex_bitmap {
    /** Address of this structure on the persistent storage. */
    size_t head;
    /** Size of the bitmap in bytes. */
    size_t size;
    /** Address of the bitmaps data. */
    size_t address;
    /** Maximum number of allocatable items. */
    size_t max_items;
    /** Number of allocated blocks. */
    size_t allocated;
    /** Index of the block that was allocated as last. */
    size_t last;
};

/** The super block.
 *
 * It's written to the offset 0 on the persistent device.
 */
struct ex_super_block {
    /** Address of the root inode. */
    inode_address root;
    /** Size of a device (in bytes). */
    size_t device_size;
    /** Data allocation bitmap. */
    struct ex_bitmap bitmap;
    /** Inode allocation bitmap. */
    struct ex_bitmap inode_bitmap;
    /** Magic number for fs checking. */
    uint32_t magic;
};

/** Representation of continuous memory of fixed size. */
struct ex_span {
    /** Data of the span. */
    const char *data;
    /** Length of data. */
    const size_t datalen;
};

/** Super block loaded to the memory.
 *
 * It must loaded for almost all operations.
 */
extern struct ex_super_block *super_block;

/** Mark nth allocatable block as free. */
void ex_bitmap_free_bit(struct ex_bitmap *bitmap, size_t nth_bit);

/** Try to find free block. */
size_t ex_bitmap_find_free_bit(struct ex_bitmap *bitmap);

/** Try to allocate data block. */
ex_status ex_super_allocate_data_block(struct ex_inode_block *block);

/** Deallocate data block. */
void ex_super_deallocate_block(block_address address);

/** Try to allocate data block. */
ex_status ex_super_allocate_inode_block(struct ex_inode_block *block);

/** Deallocate data block. */
void ex_super_deallocate_inode_block(size_t inode_number);

/** Print the super block to the stdout. */
void ex_super_print(const struct ex_super_block *block);

/** Load the super block from the perstitent storage */
ex_status ex_super_load(void);

/** Fill the statbuf */
void ex_super_statfs(struct statvfs *statbuf);

/** Check if pathname is suitable for filename */
int ex_super_check_path_len(const char *pathname);

/** Lock the inode. */
void ex_super_lock(void);

/** Unlock the inode. */
void ex_super_unlock(void);

#endif
