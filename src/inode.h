/**
 * @file inode.h
 *
 * This file provides an inode structure and the inodes' API.
 */
#ifndef EX_INODE_H
#define EX_INODE_H

#include "errors.h"
#include "util.h"
#include "super.h"
#include "path.h"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/** Number of direct blocks in the inode. */
#define EX_DIRECT_BLOCKS 256

/** Inode magic constant used for sanity check. */
extern const uint16_t EX_INODE_MAGIC1;

/** Directory magic constant used for sanity check. */
extern const uint8_t EX_DIR_MAGIC1;

/** Directory entry magic constant used for sanity check. */
extern const uint8_t EX_ENTRY_MAGIC1;

/** Maximum size of extended attribute and its value (including : and null byte). */
#define EX_INODE_ATTRIBUTE_SIZE 64

/** Size reserved for all extended attributes, */
#define EX_INODE_ATTRIBUTES_SIZE 512

/** Maximum number of extended attributes. */
#define EX_INODE_MAX_ATTRIBUTES EX_INODE_ATTRIBUTES_SIZE / EX_INODE_ATTRIBUTE_SIZE

/** This class represents the inode store on the persistent storage.
 *
 * Most of the attributes has the same meaning as in the inode(7).
 */
struct ex_inode {
    /** Number of inode.
     *
     * It it corresponds with allocated block number e.g. number == 0
     * means that inode has allocated first inode block.
     */
    size_t number;

    /** File type and mode. */
    uint32_t mode;

    /** Inode magic number. */
    uint16_t magic;

    /** User id of owner. */
    uid_t uid;

    /** Group id of owner. */
    gid_t gid;

    /** Last modification timestamp. */
    struct timespec mtime;

    /** Last access timestamp. */
    struct timespec atime;

    /** Last status change timestamp. */
    struct timespec ctime;

    /** Address of the inode on the persistent storage. */
    inode_address address;

    /** Number of hardlinks. */
    uint16_t nlinks;

    /** Size of the inodes data.
     *
     * It has two meanings:
     * 1. Size of file when inode is file
     * 2. size of file metadata (struct ex_inode) if inode is directory
     */
    size_t size;

    /** Inode data.
     * If an inode is file content is saved here directly in these blocks
     * If an inode is directory ex_dir_entries are saved in these blocks
     */
    block_address blocks[EX_DIRECT_BLOCKS];

    /** Address of block that contains extended attributes. */
    char attributes[EX_INODE_ATTRIBUTES_SIZE];

    /** Number of attributes. */
    uint8_t number_of_attributes;
};

static_assert(
    sizeof(struct ex_inode) <= EX_BLOCK_SIZE,
    "Size of the struct ex_inode must be less than EX_BLOCK_SIZE"
);

/** Maximum size of attributes name. */
#define EX_INODE_ATTR_NAME_MAX_SIZE 20

/** Maximum size of attributes' value. */
#define EX_INODE_ATTR_VALUE_MAX_SIZE 32

/** This struct represents extended attribute.
 *
 * We are doing padding by ourselves and that is the reason why we are using
 * __packed__ attribute.
 */
struct __attribute__((__packed__)) ex_inode_attribute {
    // TODO: use bit struct for namelen, valuelen and is_ocupied

    /** Length of attributes' name. */
    uint8_t namelen;
    char __padding0[3];

    /** Length of attributes' value. */
    uint8_t valuelen;
    char __pading1[3];

    /** This tells us if attribute is in use. */
    uint8_t in_use;
    char __padding2[3];

    /** Name of the attribute. */
    char name[EX_INODE_ATTR_NAME_MAX_SIZE];

    /** Value of the attribute. */
    char value[EX_INODE_ATTR_VALUE_MAX_SIZE];
};

static_assert(
    sizeof(struct ex_inode_attribute) == EX_INODE_ATTRIBUTE_SIZE,
    "Size of struct ex_inode_attribute must be same as EX_INODE_ATTRIBUTE_SIZE"
);


/** Directory entry. */
struct ex_dir_entry {
    /** Address of the entry on the persistent storage. */
    inode_address address;
    /** Entry magic. */
    uint8_t magic;
    /** Is it free? */
    uint8_t free;
    /** Name of the entry. */
    char name[EX_NAME_LEN];
};

/** Create the root on the open device
 *
 * It initializes `root` attribute of super block, so super block must
 * be loaded in the memory.
 */
ex_status ex_root_create(void);

/** Load the root from the persistent volume.
 *
 * The roots' address is readed from super block, so super block must be
 * loaded in the memory.
 */
ex_status ex_root_load(struct ex_inode *root);

/** Try to allocate direct blocks. */
ex_status ex_inode_allocate_blocks(struct ex_inode *inode);

/** Deallocate all direct blocks and block used by the inode. */
void ex_inode_deallocate_blocks(struct ex_inode *inode);

/** Free memory used by inode. */
void ex_inode_free(struct ex_inode *inode);

/** Print an inode. */
void ex_inode_print(const struct ex_inode *inode);

/** Write inodes' changes to the persitent storage. */
ex_status ex_inode_flush(const struct ex_inode *inode);

/** Return maximum number of inodes blocks. */
size_t ex_inode_max_blocks(void);

/** Copy runtime representation of a directory entry. */
struct ex_dir_entry *ex_dir_entry_copy(const struct ex_dir_entry *entry);

/** Free directory entry. */
void ex_dir_entry_free(struct ex_dir_entry *entry);

/** Copy runtime representation of an inode. */
struct ex_inode *ex_inode_copy(const struct ex_inode *inode);

/** Create an inode.
 *
 * It allocates the inode block and all its direct blocks.
 *
 * It flushes changes to the persistent storage.
 */
ex_status ex_inode_create(struct ex_inode *, uint16_t mode, gid_t gid,
                          uid_t uid);

/** Try to put an inode to the directory inode. */
struct ex_inode *ex_inode_set(struct ex_inode *dir, const char *name,
                              struct ex_inode *inode);

/** Load an inode from the persistent storage. */
ex_status ex_inode_load(inode_address ino_addr, struct ex_inode *inode);

/** Try to find an inode from the persitent storage. */
struct ex_inode *ex_inode_find(struct ex_path *path);

/** Try to obtain the inode from the directory. */
struct ex_inode *ex_inode_get(struct ex_inode *dir, const char *name);

/** Set `.` and `..` to the directory. */
void ex_inode_fill_dir(struct ex_inode *inode, struct ex_inode *parent);

/** Try to remove the inode from the directory.
 *
 * It will unlink the inode if it's hardlink reaches 0.
 */
ex_status ex_inode_unlink(struct ex_inode *dir, const char *name);

/** Get all directory entries. */
struct ex_dir_entry **ex_inode_get_all(struct ex_inode *inode);

/** Rename the inode. */
int ex_inode_rename(struct ex_inode *from_inode, struct ex_inode *to_inode,
                    const char *from_name, const char *to_name);

/** Write data to the inode. */
ssize_t ex_inode_write(struct ex_inode *inode, size_t off, const char *data,
                       size_t amount);

/** Read data from the inode. */
ex_status ex_inode_read(ssize_t *readed, struct ex_inode *ino, size_t off,
                        char *buffer, size_t amount);

/** Check that inode has required permissions. */
int ex_inode_has_perm(struct ex_inode *ino, ex_permission perm, gid_t gid,
                      uid_t uid);

/** Set extended attribute. */
int ex_inode_setxattr(struct ex_inode *, const struct ex_span *name, const struct ex_span *value);

struct ex_block_iterator {
    struct ex_inode_block last_block;
    size_t block_number;
    char buffer[EX_BLOCK_SIZE];
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

/** Iterate through all blocks of the inode */
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

/** Iterate through all directory entries */
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
