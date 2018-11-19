#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "ex.h"
#include "logging.h"

struct options {
    char *device;
    size_t inode;
    int print_super;
    int print_info;
};

char* readable_size(double size) {

    static const char* units[] = {
        "B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
    };

    static char buf[512];

    int i = 0;

    while (size > 1024) {
        size /= 1024;
        i++;
    }

    sprintf(buf, "%.*f%s", i, size, units[i]);

    return buf;
}
void print_bitmap(const char *name, struct ex_bitmap *bitmap) {

    printf("%s:\n", name);
    printf("\thead = %lu\n", bitmap->head);
    printf("\taddress = %lu\n", bitmap->address);
    printf("\tsize = %lu\n", bitmap->size);
    printf("\tallocated = %lu\n", bitmap->allocated);
}

void print_super(const char *device) {

    ex_set_log_level(warning);
    ex_device_open(device);
    ex_super_load();

    printf("super:\n");
    printf("\troot = %lu\n", super_block->root);
    printf("\tmagic = %x\n", super_block->magic);

    print_bitmap("data_bitmap", &super_block->bitmap);
    print_bitmap("inode_bitmap", &super_block->inode_bitmap);

}

void print_info(const char *device) {

    ex_set_log_level(error);
    ex_device_open(device);
    ex_super_load();

    printf("info:\n");

    size_t max_dir_entries = (EX_BLOCK_SIZE / sizeof(struct ex_dir_entry)) * (EX_DIRECT_BLOCKS - 2);
    printf("\tmax_dir_entries: %lu\n", max_dir_entries);

    printf("\tdir_entry_size: %lu\n", sizeof(struct ex_dir_entry));

    size_t max_size = EX_DIRECT_BLOCKS * 4096;
    printf("\tmax_file_size: %lu (%s)\n", max_size, readable_size(max_size));

    printf("\tinode_size: %lu\n", sizeof(struct ex_inode));
    printf("\tsuper_block_size: %lu\n", sizeof(struct ex_super_block));


}

void print_directory_entries(struct ex_inode *inode) {

    foreach_inode_block(inode, block) {
        foreach_block_entry(block, entry) {

            if(entry->free || entry->magic != EX_DIR_MAGIC1) {
                continue;
            }

            printf("\t\tname: '%s', address: %lu\n", entry->name, entry->address);
        }
    }
}

void print_inode(const char *device, size_t address) {

    ex_set_log_level(info);
    ex_device_open(device);

    struct ex_inode *inode = ex_inode_load(address);

    printf("inode:\n");

    if(!inode) {
        printf("\tno inode at %lu\n", address);
        return;
    }

    printf("\tmode: %u (", inode->mode);
    ex_print_mode(inode->mode);
    printf(")\n");

    printf("\tmagic: %x\n", inode->magic);
    printf("\tparent: %lu\n", inode->parent_inode);
    printf("\taddress: %lu\n", inode->address);
    printf("\tnlinks: %u\n", inode->nlinks);
    printf("\tino = %lu\n", inode->number);

    if(inode->mode & S_IFDIR) {
        printf("\tentries:\n");
        print_directory_entries(inode);
    }

}

void help(void) {
    printf("exdbg: \n"
        "\t--device\t\tspecify ex device\n"
        "\t--inode addr\t\tdisplay information about inode\n"
        "\t--super\t\t\tdisplay info about super block\n"
        "\t--info\t\t\tdisplay info about ex filesystem\n");
}

int main(int argc, char** argv) {

    const struct option longopts[] = {
        {"device", required_argument, 0, 'd'},
        {"inode", required_argument, 0, 'i'},
        {"super", no_argument, 0, 's'},
        {"info", no_argument, 0, 'I'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    struct options options;
    memset(&options, 0, sizeof(struct options));

    while((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
        switch(opt) {
            case 'd':
                options.device = strdup(optarg);
                break;
            case 'i':
                options.inode = strtoull(optarg, NULL, 0);
                break;
            case 's':
                options.print_super = 1;
                break;
            case 'I':
                options.print_info = 1;
                break;
            case 'h':
                help();
                return 0;
            default:
                help();
                return 1;
        }
    }

    if(!options.device) {
        printf("error: no device name supplied\n");
        help();
        return 1;
    }

    if(options.print_super) {
        print_super(options.device);
    } else if(options.print_info) {
        print_info(options.device);
    } else if(options.inode) {
        print_inode(options.device, options.inode);
    } else {
        printf("error: neither --inode | --super | --info was specified\n");
        help();
        return 1;
    }

    return 0;
}
