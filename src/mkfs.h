#include "ex.h"
#include "errors.h"
#include "logging.h"
#include <stdint.h>

// device layout:
// super_block | inode_bitmap | data_bitmap | inode_blocks | data_blocks

struct ex_mkfs_params {
    char *device;
    size_t device_size;
    size_t number_of_inodes;
    int create;
};

struct ex_mkfs_context {
    ssize_t free_device_space;
    struct ex_bitmap inode_bitmap;
    struct ex_bitmap data_bitmap;
    struct ex_super_block super_block;
};

int ex_mkfs_check_device(struct ex_mkfs_params *params);
ex_status ex_mkfs_device_clear(size_t off, size_t amount);
size_t round_block(size_t size);
int ex_mkfs_check_dbitmap_params(struct ex_mkfs_params *params,
                                 struct ex_mkfs_context *ctx);
int ex_mkfs_dbitmap_create(struct ex_mkfs_params *params,
                           struct ex_mkfs_context *ctx);
int ex_mkfs_check_ibitmap_params(struct ex_mkfs_params *params,
                                 struct ex_mkfs_context *ctx);
int ex_mkfs_ibitmap_create(struct ex_mkfs_params *params,
                           struct ex_mkfs_context *ctx);
int ex_mkfs_put_layout(const char *device, struct ex_mkfs_context *ctx);
int ex_mkfs_create_root(struct ex_mkfs_context *ctx);
int ex_mkfs_create_super_block(struct ex_mkfs_params *params,
                               struct ex_mkfs_context *ctx);
int ex_mkfs_prepare(struct ex_mkfs_params *params, struct ex_mkfs_context *ctx);
int ex_mkfs_test_init(void);
int ex_mkfs(struct ex_mkfs_params *params);
size_t ex_mkfs_get_size_for_inodes(size_t ninodes);
void ex_mkfs_check_params(struct ex_mkfs_params *params);

typedef enum {
    EX_MKFS_OPTION_OK,
    EX_MKFS_OPTION_HELP,
    EX_MKFS_OPTION_PARSE_ERROR,
    EX_MKFS_OPTION_UNKNOWN,
} ex_mkfs_parse_status;

int ex_mkfs_parse_options(struct ex_mkfs_params *params, int argc, char **argv);
void ex_mkfs_show_help(void);
