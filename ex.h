#ifndef EX_H
#define EX_H

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <assert.h>

#define EX_DEVICE "exdev"
#define EX_DIRECT_BLOCKS 16
#define EX_BLOCK_SIZE 4096
#define EX_INODE_MAGIC1 0xabcc
#define EX_DIR_MAGIC1 0xde
#define EX_NAME_LEN 54

#define info(fmt, ...) printf("%s: %s: " fmt "\n", \
        __FILE__, __func__, ##__VA_ARGS__)

typedef size_t inode_address;
typedef size_t block_address;

typedef char ex_block[EX_BLOCK_SIZE];

struct ex_dir_entry {
    inode_address address;
    uint8_t magic;
    uint8_t free;
    // XXX: name should not be here, all information should be readed from address
    char name[EX_NAME_LEN];
};

struct ex_inode {
    // the same as stat.mode_t
    uint32_t mode;
    uint16_t magic;

    time_t mtime;

    inode_address parent_inode;
    inode_address address;

    char name[EX_NAME_LEN];

    // size of file when inode is file
    // size of file metadata (struct ex_inode) if inode is directory
    size_t size;

    // if an inode is file content is saved here in these blocks
    // if an inode is directory ex_dir_entries are saved in these blocks
    // every block is EX_DIRECT_BLOCKS size
    block_address blocks[EX_DIRECT_BLOCKS];
};

struct ex_super_block {
    // address of root inode
    inode_address root;

    size_t device_size;

    // address of block bitmap
    block_address bitmap;
    size_t bitmap_size;
};

extern struct ex_super_block *super_block;
extern struct ex_inode *root;

void ex_init(void);
void ex_deinit(void);

int ex_get_device_fd(void);
void ex_device_load(void);
void ex_device_populate(size_t size);
int ex_device_create(const char *name, size_t size);
int ex_device_open(const char *device_name);

void *ex_read_device(size_t off, size_t amount);
void ex_write_device(size_t off, const char *data, size_t amount);

// add inode to to dir, write changes to device, return device ino device offset
inode_address ex_dir_set_inode(struct ex_inode *dir, struct ex_inode *ino);
// get inode from
inode_address ex_dir_get_inode(struct ex_inode *dir, const char *name);
struct ex_inode *ex_dir_load_inode(struct ex_inode *ino, const char *name);
struct ex_inode *ex_dir_remove_inode(struct ex_inode *dir, const char *name);
struct ex_inode **ex_dir_get_inodes(struct ex_inode *ino);

void ex_inode_write(struct ex_inode *ino, size_t off, const char *data, size_t amount);
char *ex_inode_read(struct ex_inode *ino, size_t off, size_t amount);
struct ex_inode *ex_inode_create(char *name, uint16_t mode);

void ex_print_inode(const struct ex_inode *inode);
// load inode from given address
struct ex_inode *ex_inode_load(inode_address ino_addr);

block_address ex_allocate_block(void);
void ex_print_super_block(const struct ex_super_block *block);

#define foreach_inode_block(inode, block) \
    block_address block##_addr = 0; \
    char *block = NULL; \
    for(size_t block##_no = 0; \
            block##_no < EX_DIRECT_BLOCKS && \
            (block##_addr = inode->blocks[block##_no], block ? free(block): 1,\
            block = ex_read_device(block##_addr, EX_BLOCK_SIZE), 1); \
            block##_no++) \

#define foreach_block_entry(block, entry) \
    struct ex_dir_entry *entry = NULL; \
    for(size_t entry##_no = 0; \
            entry##_no < EX_BLOCK_SIZE / sizeof(struct ex_dir_entry) && \
            (entry = (struct ex_dir_entry *)&block[entry##_no], 1); \
            entry##_no += sizeof(struct ex_dir_entry))

#endif /* EX_H */
