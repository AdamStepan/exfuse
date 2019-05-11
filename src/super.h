#ifndef EX_SUPER_H
#define EX_SUPER_H

#include "device.h"
#include "logging.h"
#include "path.h"
#include "util.h"
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/statvfs.h>

// how large block will be
#define EX_BLOCK_SIZE 4096
// maximum filename basename length
#define EX_NAME_LEN 54
#define EX_SUPER_MAGIC 0xffaacc

#define EX_BLOCK_INVALID_ID ((size_t)-1)
#define EX_BLOCK_INVALID_ADDRESS ((size_t)-1)

typedef size_t inode_address;
typedef size_t block_address;

struct ex_inode_block {
    // position in super block inodes block bitmap
    size_t id;
    // address of inode block
    size_t address;
    // block data, not loaded by default
    char *data;
};

struct ex_bitmap {
    // where on disk is *this* structure stored
    size_t head;
    // size of the bitmap in bytes
    size_t size;
    // address of a bitmap
    size_t address;
    // maximum number of items in a bitmap
    size_t max_items;
    // number of allocated blocks
    size_t allocated;
    size_t last;
};

struct ex_super_block {
    // address of root inode
    inode_address root;
    // size of a device
    size_t device_size;
    // data allocation bitmap
    struct ex_bitmap bitmap;
    // inode allocation bitmap
    struct ex_bitmap inode_bitmap;
    // magic number for fs checking
    uint32_t magic;
};

extern struct ex_super_block *super_block;

void ex_bitmap_free_bit(struct ex_bitmap *bitmap, size_t nth_bit);
size_t ex_bitmap_find_free_bit(struct ex_bitmap *bitmap);

block_address ex_super_allocate_block(void);
void ex_super_deallocate_block(block_address address);

struct ex_inode_block ex_super_allocate_inode_block(void);
void ex_super_deallocate_inode_block(size_t inode_number);

void ex_super_print(const struct ex_super_block *block);
void ex_super_load(void);
void ex_super_statfs(struct statvfs *statbuf);
int ex_super_check_path_len(const char *pathname);

void ex_super_lock(void);
void ex_super_unlock(void);

#endif
