#ifndef EX_SUPER_H
#define EX_SUPER_H

#include <stdint.h>
#include <stdlib.h>
#include <logging.h>
#include <device.h>
#include <path.h>
#include <util.h>

// how large block will be
#define EX_BLOCK_SIZE 4096

typedef size_t inode_address;
typedef size_t block_address;

struct ex_super_block {
    // address of root inode
    inode_address root;
    // TODO: add magic, add fs info
    size_t device_size;

    // address of block bitmap
    block_address bitmap;
    size_t bitmap_size;
};

extern struct ex_super_block *super_block;

block_address ex_super_allocate_block(void);
void ex_super_deallocate_block(block_address address);

void ex_super_print(const struct ex_super_block *block);
void ex_super_write(size_t device_size);
void ex_super_load(void);

#endif
