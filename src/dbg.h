#include <stddef.h>
#include "super.h"
#include "inode.h"

enum ex_dbg_action {
    PRINT_SUPER,
    PRINT_INFO,
    PRINT_INODE,
    PRINT_INODE_DATA,
    PRINT_BITMAP_DATA,
    PRINT_SIZES,
};

struct ex_dbg_options {
    char *device;
    size_t inode;
    size_t inode_data;
    size_t bitmap_data;
    int print_super;
    int print_info;
    enum ex_dbg_action action;
};

void ex_dbg_print_struct_sizes(void);
void ex_dbg_print_bitmap(const char *name, struct ex_bitmap *bitmap);
void ex_dbg_print_bitmap_data(const char *device, size_t head);
void ex_dbg_print_super(const char *device);
void ex_dbg_print_info(const char *device);
void ex_dbg_print_directory_entries(struct ex_inode *inode);
void ex_dbg_print_inode_data(const char *device, size_t address);
void ex_dbg_print_inode_attrs(const struct ex_inode *inode);
void ex_dbg_print_inode(const char *device, size_t address);
void ex_dbg_help(void);
int ex_dbg_parse_options(struct ex_dbg_options *, int argc, char **argv);
int ex_dbg_run(struct ex_dbg_options *);
