#include "ex.h"
#include "logging.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

enum action {
    PRINT_SUPER,
    PRINT_INFO,
    PRINT_INODE,
    PRINT_INODE_DATA,
    PRINT_BITMAP_DATA
};

struct options {
    char *device;
    size_t inode;
    size_t inode_data;
    size_t bitmap_data;
    int print_super;
    int print_info;
    enum action action;
};

char *readable_size(double size) {

    static const char *units[] = {"B",   "kiB", "MiB", "GiB", "TiB",
                                  "PiB", "EiB", "ZiB", "YiB"};

    static char buf[512];

    int i = 0;

    while (size >= 1024) {
        size /= 1024;
        i++;
    }

    sprintf(buf, "%.*f%s", i, size, units[i]);

    return buf;
}
void print_bitmap(const char *name, struct ex_bitmap *bitmap) {

    printf("%s:\n", name);
    printf("\thead = %lu\n", bitmap->head);
    printf("\tlast = %lu\n", bitmap->last);
    printf("\taddress = %lu\n", bitmap->address);
    printf("\tsize = %lu\n", bitmap->size);
    printf("\tallocated = %lu\n", bitmap->allocated);
    printf("\tmaxitems = %lu\n", bitmap->max_items);
}

void print_bitmap_data(const char *device, size_t head) {

    ex_set_log_level(warning);
    ex_device_open(device);
    ex_super_load();

    struct ex_bitmap *bitmap = ex_device_read(head, sizeof(struct ex_bitmap));
    char *bitmap_data = ex_device_read(bitmap->address, bitmap->size);

    write(fileno(stdout), bitmap_data, bitmap->size);
}

void print_super(const char *device) {

    ex_set_log_level(warning);
    ex_device_open(device);
    ex_super_load();

    printf("super:\n");
    printf("\troot = %lu\n", super_block->root);
    printf("\tmagic = %x\n", super_block->magic);
    printf("\tdevice_size = %lu (%s)\n", super_block->device_size,
           readable_size(super_block->device_size));
    print_bitmap("data_bitmap", &super_block->bitmap);
    print_bitmap("inode_bitmap", &super_block->inode_bitmap);
}

void print_info(const char *device) {

    ex_set_log_level(error);
    ex_device_open(device);
    ex_super_load();

    printf("info:\n");

    size_t max_dir_entries =
        (EX_BLOCK_SIZE / sizeof(struct ex_dir_entry)) * (EX_DIRECT_BLOCKS - 2);
    printf("\tmax_dir_entries: %lu\n", max_dir_entries);

    printf("\tdir_entry_size: %luB\n", sizeof(struct ex_dir_entry));

    size_t max_size = EX_DIRECT_BLOCKS * 4096;
    printf("\tmax_file_size: %luB (%s)\n", max_size, readable_size(max_size));

    printf("\tinode_size: %luB\n", sizeof(struct ex_inode));
    printf("\tsuper_block_size: %luB\n", sizeof(struct ex_super_block));
}

void print_directory_entries(struct ex_inode *inode) {

    foreach_inode_block(inode, block) {
        foreach_block_entry(block, entry) {

            if (entry.free || entry.magic != EX_DIR_MAGIC1) {
                continue;
            }

            printf("\t\tname: '%s', address: %lu\n", entry.name, entry.address);
        }
    }
}

void print_inode_data(const char *device, size_t address) {

    ex_set_log_level(error);
    ex_device_open(device);

    struct ex_inode *inode = ex_inode_load(address);

    if (!inode) {
        printf("\tno inode at %lu\n", address);
        return;
    }

    foreach_inode_block(inode, block) {
        write(fileno(stdout), block.data, EX_BLOCK_SIZE);
    }

    foreach_inode_block_cleanup(inode, block);
}

void print_inode(const char *device, size_t address) {

    ex_set_log_level(info);
    ex_device_open(device);

    struct ex_inode *inode = ex_inode_load(address);

    if (!inode) {
        printf("\tno inode at %lu\n", address);
        return;
    }

    printf("inode:\n");

    printf("\tnumber: %lu\n", inode->number);
    printf("\tsize: %lu\n", inode->size);
    printf("\tmagic: %x\n", inode->magic);
    printf("\taddress: %lu\n", inode->address);
    printf("\tnlinks: %u\n", inode->nlinks);

    printf("\tmode: %o (", inode->mode);
    ex_print_mode(inode->mode);
    printf(")\n");

    printf("\tmtime: %ld.%.9ld\n", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    printf("\tatime: %ld.%.9ld\n", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    printf("\tctime: %ld.%.9ld\n", inode->mtime.tv_sec, inode->mtime.tv_nsec);

    if (inode->mode & S_IFDIR) {
        printf("\tentries:\n");
        print_directory_entries(inode);
    }
}

void help(void) {
    printf("exdbg: \n"
           "\t--device\t\tspecify ex device\n"
           "\t--inode addr\t\tdisplay information about inode\n"
           "\t--inode-data\t\tdisplay inode data (binary)\n"
           "\t--bitmap-data\t\tdisplay bitmap data\n"
           "\t--super\t\t\tdisplay info about super block\n"
           "\t--info\t\t\tdisplay info about ex filesystem\n");
}

int main(int argc, char **argv) {

    const struct option longopts[] = {
        {"device", required_argument, 0, 'd'},
        {"inode", required_argument, 0, 'i'},
        {"inode-data", required_argument, 0, 'D'},
        {"bitmap-data", required_argument, 0, 'b'},
        {"super", no_argument, 0, 's'},
        {"info", no_argument, 0, 'I'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opt;
    struct options options;
    memset(&options, '\0', sizeof(options));

    while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
        switch (opt) {
        case 'b':
            options.bitmap_data = strtoull(optarg, NULL, 0);
            options.action = PRINT_BITMAP_DATA;
            break;
        case 'd':
            options.device = strdup(optarg);
            break;
        case 'D':
            options.inode_data = strtoull(optarg, NULL, 0);
            options.action = PRINT_INODE_DATA;
            break;
        case 'i':
            options.inode = strtoull(optarg, NULL, 0);
            options.action = PRINT_INODE;
            break;
        case 's':
            options.print_super = 1;
            options.action = PRINT_SUPER;
            break;
        case 'I':
            options.print_info = 1;
            options.action = PRINT_INFO;
            break;
        case 'h':
            help();
            return 0;
        default:
            help();
            return 1;
        }
    }

    if (!options.device) {
        printf("error: no device name supplied\n");
        help();
        return 1;
    }

    switch (options.action) {
    case PRINT_SUPER:
        print_super(options.device);
        break;
    case PRINT_INFO:
        print_info(options.device);
        break;
    case PRINT_INODE:
        print_inode(options.device, options.inode);
        break;
    case PRINT_INODE_DATA:
        print_inode_data(options.device, options.inode_data);
        break;
    case PRINT_BITMAP_DATA:
        print_bitmap_data(options.device, options.bitmap_data);
        break;
    default:
        print_super(options.device);
    }

    return 0;
}
