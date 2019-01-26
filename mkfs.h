#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

#include "ex.h"
#include "logging.h"

// device layout:
// super_block | inode_bitmap | data_bitmap | inode_blocks | data_blocks

struct ex_mkfs_params {
    char *device;
    size_t device_size;
    size_t number_of_inodes;
};

struct ex_mkfs_context {

    ssize_t free_device_space;

    struct ex_bitmap inode_bitmap;
    struct ex_bitmap data_bitmap;
    struct ex_super_block super_block;

};

char* readable_size(double size) {

    static const char* units[] = {
        "B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
    };

    static char buf[512];

    int i = 0;

    while (size >= 1024) {
        size /= 1024;
        i++;
    }

    sprintf(buf, "%.*f%s", i, size, units[i]);

    return buf;
}

int ex_mkfs_check_device(struct ex_mkfs_params *params) {

    if (access(params->device, R_OK | W_OK)) {
        error("access(%s): errno %s", params->device, strerror(errno));
        return -EACCES;
    }

    if (!params->device_size) {

        struct stat buf;

        if (stat(params->device, &buf)) {
            int rv = errno;
            error("stat(%s): unable to get device size: %i", params->device, rv);
            return -rv;
        }

        params->device_size = buf.st_size;
    }

    return 0;
}

int ex_mkfs_device_clear(size_t off, size_t amount) {

    char *buffer = mmap(NULL, amount, PROT_WRITE, MAP_SHARED, ex_device_fd(), off);

    if (buffer == MAP_FAILED) {
        fatal("mmap failed: off=%zu, amount=%zu, error: %s",
               off, amount, strerror(errno));
    }

    memset(buffer, '\0', amount);

    // XXX: you know what to do
    return 0;
}

// XXX: use c11 macro
size_t round_block(size_t size) {
    return ceil((double)size / EX_BLOCK_SIZE) * EX_BLOCK_SIZE;
}

ssize_t round_block_signed(ssize_t size) {
    return ceil((double)size / EX_BLOCK_SIZE) * EX_BLOCK_SIZE;
}

int ex_mkfs_check_dbitmap_params(struct ex_mkfs_params *params,
                                 struct ex_mkfs_context *ctx) {

    // XXX: this works only because we allocate all blocks when we create
    //      an inode, generally can be true that ex_inode_blocks can be
    //      greater than a device size
    size_t data_size = params->number_of_inodes * ex_inode_max_blocks();
    size_t dbitmap_size = round_block(params->number_of_inodes / 8);

    if (ctx->free_device_space < 0) {
        goto enospc;
    }

    if (data_size + dbitmap_size > (size_t)ctx->free_device_space) {
        goto enospc;
    }

    return 0;

enospc:
    error("not enough space for data and dbitmap, got: %zi, need: %zu",
          ctx->free_device_space, data_size + dbitmap_size);
    return -ENOSPC;
}

int ex_mkfs_dbitmap_create(struct ex_mkfs_params *params,
                           struct ex_mkfs_context *ctx) {

    if (ex_mkfs_check_dbitmap_params(params, ctx)) {
        return -EINVAL;
    }

    ctx->data_bitmap.allocated = 0;
    // data bitmap starts after end of an ibitmap
    ctx->data_bitmap.address = ctx->inode_bitmap.address + \
                               round_block(ctx->inode_bitmap.size);
    ctx->data_bitmap.head = offsetof(struct ex_super_block, bitmap);

    ctx->data_bitmap.max_items = params->number_of_inodes * ex_inode_max_blocks();
    ctx->data_bitmap.size = ctx->data_bitmap.max_items / 8;

    ctx->free_device_space -= ctx->data_bitmap.max_items * EX_BLOCK_SIZE;
    ctx->free_device_space -= round_block(ctx->data_bitmap.size);

    return 0;
}

int ex_mkfs_check_ibitmap_params(struct ex_mkfs_params *params,
                                 struct ex_mkfs_context *ctx) {

    if (params->number_of_inodes % 8) {
        error("number of inodes must be divisible by 8");
        return -EINVAL;
    }

    ssize_t inodes_space = EX_BLOCK_SIZE * params->number_of_inodes;
    ssize_t bitmap_space = params->number_of_inodes / 8;
    ssize_t needed_space = inodes_space + bitmap_space;

    debug("inodes_space: %zd, bitmap_space: %zd, free_space: %zd",
          inodes_space, bitmap_space, ctx->free_device_space);

    if (needed_space > ctx->free_device_space) {
        error("not enough space for inode bitmap: got %zd, need: %zd",
              ctx->free_device_space, needed_space);
        return -EINVAL;
    }

    return 0;
}

int ex_mkfs_ibitmap_create(struct ex_mkfs_params *params,
                           struct ex_mkfs_context *ctx) {

    if (ex_mkfs_check_ibitmap_params(params, ctx)) {
        return -EINVAL;
    }

    ctx->inode_bitmap.allocated = 0;
    ctx->inode_bitmap.address = round_block(sizeof(struct ex_super_block));
    ctx->inode_bitmap.head = offsetof(struct ex_super_block, inode_bitmap);

    ctx->inode_bitmap.max_items = params->number_of_inodes;
    ctx->inode_bitmap.size = ctx->inode_bitmap.max_items / 8;

    // adjust free device space
    ctx->free_device_space -= ctx->inode_bitmap.max_items * EX_BLOCK_SIZE;
    ctx->free_device_space -= round_block(ctx->inode_bitmap.size);

    return 0;
}

int ex_mkfs_put_layout(const char *device, struct ex_mkfs_context *ctx) {

    if (!ex_device_open(device)) {
        return 1;
    }

    debug("clearing super block space");
    // clean super block space
    size_t size = round_block(sizeof(ctx->super_block));
    ex_mkfs_device_clear(0, size);

    debug("clearing inode bitmap space");
    // clean inode bitmap space
    size = round_block(ctx->inode_bitmap.size);
    ex_mkfs_device_clear(ctx->inode_bitmap.address, size);

    debug("clearing data bitmap space");
    // clean data bitmap space
    size = round_block(ctx->data_bitmap.size);
    ex_mkfs_device_clear(ctx->data_bitmap.address, size);

    debug("writing super block");
    // write super block
    size = sizeof(ctx->super_block);
    ex_device_write(0, (char *)&ctx->super_block, size);

    // XXX: check writes
    return 0;
}

int ex_mkfs_create_root(struct ex_mkfs_context *ctx) {
    // now super block should be written and device should be
    // open, so we can create root, but first we need to load
    // super block into memory
    (void)ctx;

    ex_super_load();
    ex_root_write();

    return 0;
}

int ex_mkfs_create_super_block(struct ex_mkfs_params *params,
                               struct ex_mkfs_context *ctx) {

    ctx->super_block = (struct ex_super_block) {
        // we can set root until we put super block on disk
        .root = 0,
        .device_size = params->device_size,
        .bitmap = ctx->data_bitmap,
        .inode_bitmap = ctx->inode_bitmap,
        .magic = EX_SUPER_MAGIC
    };

    // XXX: we should do at least some checks
    return 0;
}

int ex_mkfs_prepare(struct ex_mkfs_params *params, struct ex_mkfs_context *ctx) {

    // adjust avaible free space
    debug("available free space: %zi", ctx->free_device_space);
    ctx->free_device_space -= round_block(sizeof(struct ex_super_block));

    // create inode bitmap
    debug("available free space: %zi", ctx->free_device_space);
    int rv = ex_mkfs_ibitmap_create(params, ctx);

    if (rv) {
        error("unable to create inode bitmap");
        goto end;
    }

    // create data bitmap
    debug("available free space: %zi", ctx->free_device_space);
    rv = ex_mkfs_dbitmap_create(params, ctx);

    if (rv) {
        error("unable to create data bitmap");
        goto end;
    }

    // create super block
    debug("available free space: %zi", ctx->free_device_space);
    rv = ex_mkfs_create_super_block(params, ctx);

    if (rv) {
        error("unable to create super block");
        goto end;
    }

end:
    return rv;

}

int ex_mkfs_test_init(void) {

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    params.device_name = "exdev";

    return ex_mkfs(&params);
}

int ex_mkfs(struct ex_mkfs_params *params) {

    int rv = ex_mkfs_check_device(params);

    if (rv) {
        error("check device failed");
        goto error;
    }

    struct ex_mkfs_context ctx;
    memset(&ctx, '\0', sizeof(ctx));
    ctx.free_device_space = params->device_size;

    rv = ex_mkfs_prepare(params, &ctx);

    if (rv) {
        error("prepare failed");
        goto error;
    }

    rv = ex_mkfs_put_layout(params->device, &ctx);

    if (rv) {
        error("unable to put layout");
        goto error;
    }

    return ex_mkfs_create_root(&ctx);
error:
    return rv;
}




void help(void) {
    printf("mkfs.exfuse: \n"
        "\t--device\t\tspecify a device name\n"
        "\t--inodes\t\tspecify maximum of inodes (default: 256)\n"
        "\t--size\t\tspecify size of a device\n");
}

size_t ex_mkfs_get_size_for_inodes(size_t ninodes) {

    // space for inodes
    size_t required = ninodes * EX_BLOCK_SIZE;
    // space for inodes bitmap
    required += round_block(ninodes / 8);
    // space for data inodes
    required += ninodes * ex_inode_max_blocks() * EX_BLOCK_SIZE;
    // space for data bitmap
    required += round_block(ninodes * ex_inode_max_blocks() / 8);
    // space for super block
    required += round_block(sizeof(struct ex_super_block));

    return required;
}

void ex_mkfs_check_params(struct ex_mkfs_params *params) {

    if (!params->device) {
        fatal("no device name supplied");
    }

    if (params->device_size && params->device_size % EX_BLOCK_SIZE) {
        fatal("device size is not divisible by %u", EX_BLOCK_SIZE);
    }

    if (!params->number_of_inodes) {
        info("number of inodes was not specified, setting to 256");
        params->number_of_inodes = 256;
    }

    if (!params->device_size) {
        params->device_size = ex_mkfs_get_size_for_inodes(params->number_of_inodes);
        info("device size was not specified, setting size to %zu",
             params->device_size);
    }

}

int main(int argc, char** argv) {

    const struct option longopts[] = {
        {"device", required_argument, 0, 'd'},
        {"inodes", required_argument, 0, 'i'},
        {"size", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    while((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
        switch(opt) {
            case 'd':
                params.device = strdup(optarg);
                break;
            case 'i':
                params.number_of_inodes = strtoull(optarg, NULL, 0);
                break;
            case 's':
                params.device_size = strtoull(optarg, NULL, 0);
                break;
            case 'h':
                help();
                return 0;
            default:
                help();
                return 1;
        }
    }

    ex_mkfs_check_params(&params);

    return ex_mkfs(&params);
}
