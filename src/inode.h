#ifndef EX_INODE_H
#define EX_INODE_H

#include "device.h"
#include "errors.h"
#include "logging.h"
#include "super.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define EX_DIRECT_BLOCKS 256

extern const uint16_t EX_INODE_MAGIC1;
extern const uint8_t EX_DIR_MAGIC1;
extern const uint8_t EX_ENTRY_MAGIC1;

struct ex_inode {
    // number of inode, it corresponds with allocated block number
    // e.g. number==0 means that inode has allocated first inode
    // block
    size_t number;

    // the same as stat.mode_t
    uint32_t mode;
    uint16_t magic;

    // modification time
    struct timespec mtime;
    // access time
    struct timespec atime;
    // change time (time when attribute was changed)
    struct timespec ctime;

    inode_address address;

    // number of links
    uint16_t nlinks;

    // size of file when inode is file
    // size of file metadata (struct ex_inode) if inode is directory
    size_t size;

    // if an inode is file content is saved here directly in these blocks
    // if an inode is directory ex_dir_entries are saved in these blocks
    block_address blocks[EX_DIRECT_BLOCKS];
};

struct ex_dir_entry {
    inode_address address;
    uint8_t magic;
    uint8_t free;
    char name[EX_NAME_LEN];
};

extern struct ex_inode *root;

ex_status ex_root_write(void);
ex_status ex_root_load(void);

#define ex_inode_update_time(rv, ino, attr)                                    \
    int rv;                                                                    \
    do {                                                                       \
        struct timespec ts;                                                    \
        rv = clock_gettime(CLOCK_MONOTONIC, &tp);                              \
        ino->attr.v_sec = rv.tv_sec;                                           \
        inode->attr.tv_nsec = rv.tv_nsec;                                      \
    } while (0);

ex_status ex_inode_allocate_blocks(struct ex_inode *inode);
void ex_inode_deallocate_blocks(struct ex_inode *inode);
void ex_inode_free(struct ex_inode *inode);
void ex_inode_print(const struct ex_inode *inode);
ex_status ex_inode_flush(const struct ex_inode *inode);
size_t ex_inode_max_blocks(void);

struct ex_dir_entry *ex_dir_entry_copy(const struct ex_dir_entry *entry);
void ex_dir_entry_free(struct ex_dir_entry *entry);

struct ex_inode *ex_inode_copy(const struct ex_inode *inode);
struct ex_inode *ex_inode_create(uint16_t mode);
struct ex_inode *ex_inode_set(struct ex_inode *dir, const char *name,
                              struct ex_inode *inode);
struct ex_inode *ex_inode_load(inode_address ino_addr);

struct ex_inode *ex_inode_find(struct ex_path *path);
struct ex_inode *ex_inode_get(struct ex_inode *dir, const char *name);
void ex_inode_fill_dir(struct ex_inode *inode, struct ex_inode *parent);
struct ex_inode *ex_inode_unlink(struct ex_inode *dir, const char *name);
struct ex_dir_entry **ex_inode_get_all(struct ex_inode *inode);
int ex_inode_rename(struct ex_inode *from_inode, struct ex_inode *to_inode,
                    const char *from_name, const char *to_name);

ssize_t ex_inode_write(struct ex_inode *inode, size_t off, const char *data,
                       size_t amount);
ssize_t ex_inode_read(struct ex_inode *inode, size_t off, char *buffer,
                      size_t amount);

struct ex_block_iterator {
    struct ex_inode_block last_block;
    size_t block_number;
};

struct ex_entry_iterator {
    size_t entry_number;
    struct ex_dir_entry last_entry;
    struct ex_inode_block block;
};

struct ex_inode_block ex_inode_block_iterate(struct ex_inode *inode,
                                             struct ex_block_iterator *it);
struct ex_dir_entry ex_inode_entry_iterate(struct ex_inode_block block,
                                           struct ex_entry_iterator *it);

#define foreach_inode_block(inode, block)                                      \
    struct ex_inode_block block = {.id = EX_BLOCK_INVALID_ID,                  \
                                   .data = NULL,                               \
                                   .address = EX_BLOCK_INVALID_ADDRESS};       \
                                                                               \
    struct ex_block_iterator block##_iterator = {.block_number = 0,            \
                                                 .last_block = block};         \
    for (; (block = ex_inode_block_iterate(inode, &block##_iterator),          \
           block.data);)

#define foreach_inode_block_cleanup(inode, block)                              \
    ex_inode_block_iterate(inode, &block##_iterator)

#define foreach_block_entry(block, entry)                                      \
    struct ex_dir_entry entry = {.address = EX_BLOCK_INVALID_ADDRESS,          \
                                 .name = {0},                                  \
                                 .free = 0,                                    \
                                 .magic = 0};                                  \
    struct ex_entry_iterator entry##_iterator = {                              \
        .entry_number = 0, .last_entry = entry, .block = block};               \
    for (; (entry = ex_inode_entry_iterate(block, &entry##_iterator),          \
           entry.magic);)

#endif
