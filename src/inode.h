#ifndef EX_INODE_H
#define EX_INODE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <device.h>
#include <super.h>
#include <logging.h>

#define EX_DIRECT_BLOCKS 256
#define EX_INODE_MAGIC1 0xabcc
#define EX_DIR_MAGIC1 0xde
// maximum filename basename length
#define EX_NAME_LEN 54

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

    // if an inode is file content is saved here directly in these blocks
    // if an inode is directory ex_dir_entries are saved in these blocks
    // every block is EX_DIRECT_BLOCKS size
    block_address blocks[EX_DIRECT_BLOCKS];
};

struct ex_dir_entry {
    inode_address address;
    uint8_t magic;
    uint8_t free;
    // XXX: name should not be here, all information should be readed from address
    char name[EX_NAME_LEN];
};

extern struct ex_inode *root;

void ex_root_write(void);
void ex_root_load(void);

/**
 * Try to allocate EX_DIRECT_BLOCKS for inode. i
 * @return 1 if EX_DIRECT_BLOCKS can be allocated, 0 otherwise.
 */
int ex_inode_allocate_blocks(struct ex_inode *inode);
void ex_inode_deallocate_blocks(struct ex_inode *inode);
void ex_inode_free(struct ex_inode *inode);
void ex_inode_print(const struct ex_inode *inode);

struct ex_inode *ex_inode_copy(const struct ex_inode *inode);
struct ex_inode *ex_inode_create(char *name, uint16_t mode);
// add inode to to dir, write changes to device, return device inode device offset
struct ex_inode *ex_inode_set(struct ex_inode *dir, struct ex_inode *inode);
// load inode from given address
struct ex_inode *ex_inode_load(inode_address ino_addr);

struct ex_inode *ex_inode_find(struct ex_inode *dir, struct ex_path *path);
struct ex_inode *ex_inode_get(struct ex_inode *dir, const char *name);
struct ex_inode *ex_inode_remove(struct ex_inode *dir, const char *name);
struct ex_inode **ex_inode_get_all(struct ex_inode *inode);

ssize_t ex_inode_write(struct ex_inode *inode, size_t off, const char *data, size_t amount);
char *ex_inode_read(struct ex_inode *inode, size_t off, size_t amount);

#define foreach_inode_block(inode, block) \
    block_address block##_addr = 0; \
    char *block = NULL; \
    for(size_t block##_no = 0; \
            (block ? (free(block), block##_no < EX_DIRECT_BLOCKS) : \
                block##_no < EX_DIRECT_BLOCKS) && \
            (block##_addr = inode->blocks[block##_no], \
            block = ex_device_read(block##_addr, EX_BLOCK_SIZE), 1); \
            block##_no++) \

#define foreach_block_entry(block, entry) \
    struct ex_dir_entry *entry = NULL; \
    for(size_t entry##_no = 0; \
            entry##_no < EX_BLOCK_SIZE / sizeof(struct ex_dir_entry) && \
            (entry = (struct ex_dir_entry *)&block[entry##_no], 1); \
            entry##_no += sizeof(struct ex_dir_entry))
#endif
