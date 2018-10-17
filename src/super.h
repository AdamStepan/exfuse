#ifndef EX_SUPER_H
#define EX_SUPER_H

#include <sys/statvfs.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <logging.h>
#include <device.h>
#include <path.h>
#include <util.h>

// how large block will be
#define EX_BLOCK_SIZE 4096
// maximum filename basename length
#define EX_NAME_LEN 54
#define EX_SUPER_MAGIC 0xffaacc

typedef size_t inode_address;
typedef size_t block_address;

struct ex_bitmap {
    // where on disk is this bitmap stored
    size_t head;
    // address of bitmap
    size_t address;
    // max number of allocated blocks
    size_t size;
    // number of allocated blocks
    size_t allocated;
};

struct ex_super_block {
    // address of root inode
    inode_address root;
    // size of whole device (sizeof(super block) + bitmap.size * 4096)
    size_t device_size;
    // data+inode allocation bitmap
    struct ex_bitmap bitmap;
    // magic number for fs checking
    uint32_t magic;
};

extern struct ex_super_block *super_block;

void ex_bitmap_free_bit(struct ex_bitmap *bitmap, size_t nth_bit);
size_t ex_bitmap_find_free_bit(struct ex_bitmap *bitmap);

block_address ex_super_allocate_block(void);
void ex_super_deallocate_block(block_address address);

void ex_super_print(const struct ex_super_block *block);
void ex_super_write(size_t device_size);
void ex_super_load(void);
void ex_super_statfs(struct statvfs *statbuf);

int ex_super_check_path_len(const char *pathname);

#endif
