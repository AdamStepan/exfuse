#include "mkfs.h"
#include "errors.h"
#include "ex.h"
#include "logging.h"
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// device layout:
// super_block | inode_bitmap | data_bitmap | inode_blocks | data_blocks

int ex_mkfs_check_device(struct ex_mkfs_params *params) {

    int rv = 0;

    if (params->create) {

        info("creating device: %s", params->device);
        int fd = open(params->device, O_CREAT, S_IRUSR | S_IWUSR);

        if (!fd) {
            rv = -errno;
            close(fd);
            error("unable to create test device");
            return rv;
        }
        close(fd);
    };

    if (access(params->device, R_OK | W_OK)) {
        error("access(%s): %s", params->device, strerror(errno));
        return -EACCES;
    }

    if (!params->device_size) {

        struct stat buf;

        if (stat(params->device, &buf)) {
            rv = errno;
            error("stat(%s): unable to get device size: %i", params->device,
                  rv);
            return -rv;
        }

        params->device_size = buf.st_size;
    }

    if (params->create) {

        char sizebuf[128];
        ex_readable_size(sizebuf, sizeof(sizebuf), params->device_size);

        info("resizing device: %s to %zu (%s)", params->device,
             params->device_size, sizebuf);

        rv = truncate(params->device, params->device_size);

        if (rv == -1) {
            rv = -errno;
            error("unable resize device");
        }
    }

    return rv;
}

ex_status ex_mkfs_device_clear(size_t off, size_t amount) {

    ex_status status = OK;
    int fd = 0;

    if ((status = ex_device_fd(&fd)) != OK) {
        goto error;
    }

    struct stat statbuf;

    if (fstat(fd, &statbuf) != 0) {
        status = DEVICE_STAT_FAILED;
        goto error;
    }

    if ((size_t)statbuf.st_size - off < amount) {
        status = ZEROING_OUTSIDE_OF_DEVICE_SPACE;
        goto error;
    }

    char *buffer = mmap(NULL, amount, PROT_WRITE, MAP_SHARED, fd, off);

    if (buffer == MAP_FAILED) {
        status = DEVICE_MMAP_FAILED;
        goto error;
    }

    memset(buffer, '\0', amount);

    return status;

error:

    switch (status) {
    case DEVICE_STAT_FAILED:
        fatal("unable to stat device: fd=%d, errno=%s", fd, strerror(errno));
        break;
    case ZEROING_OUTSIDE_OF_DEVICE_SPACE:
        fatal("specified range points outside of device file: size=%ld"
              ", off=%zu, amount=%zu",
              statbuf.st_size, off, amount);
        break;
    case DEVICE_MMAP_FAILED:
        fatal("mmap failed: off=%zu, amount=%zu, fatal: %s", off, amount,
              strerror(errno));
        break;
    default:
        fatal("unhandled error: %d", status);
    }

    return status;
}

size_t round_block(size_t size) {
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
    ctx->data_bitmap.last = 0;

    // data bitmap starts after end of an ibitmap
    ctx->data_bitmap.address =
        ctx->inode_bitmap.address + round_block(ctx->inode_bitmap.size);
    ctx->data_bitmap.head = offsetof(struct ex_super_block, bitmap);

    ctx->data_bitmap.max_items =
        params->number_of_inodes * ex_inode_max_blocks();
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

    debug("inodes_space: %zd, bitmap_space: %zd, free_space: %zd", inodes_space,
          bitmap_space, ctx->free_device_space);

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
    ctx->inode_bitmap.last = 0;

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

    if (ex_device_open(device) != OK) {
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
    ex_root_create();

    return 0;
}

int ex_mkfs_create_super_block(struct ex_mkfs_params *params,
                               struct ex_mkfs_context *ctx) {

    ctx->super_block = (struct ex_super_block){
        // we can set root until we put super block on disk
        .root = 0,
        .device_size = params->device_size,
        .bitmap = ctx->data_bitmap,
        .inode_bitmap = ctx->inode_bitmap,
        .magic = EX_SUPER_MAGIC};

    // XXX: we should do at least some checks
    return 0;
}

int ex_mkfs_prepare(struct ex_mkfs_params *params,
                    struct ex_mkfs_context *ctx) {

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

    params.device = "exdev";
    params.create = 1;

    ex_mkfs_check_params(&params);

    return ex_mkfs(&params);
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

        params->device_size =
            ex_mkfs_get_size_for_inodes(params->number_of_inodes);

        char sizebuf[128];
        ex_readable_size(sizebuf, sizeof(sizebuf), params->device_size);

        info("device size was not specified, setting size to %zu (%s)",
             params->device_size, sizebuf);
    }
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

    rv = ex_mkfs_create_root(&ctx);
error:
    if (!rv) {
        info("fs was successfully created");
    }

    return rv;
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

void ex_mkfs_show_help(void) {
    info("\n\t--device\t\tspecify a device name\n"
         "\t--inodes\t\tspecify maximum of inodes (default: 256)\n"
         "\t--size\t\t\tspecify size of a device\n"
         "\t--create\t\tcreate a device if it not exist\n"
         "\t--log-level\t\tspecify log level\n");
}

int ex_mkfs_parse_options(struct ex_mkfs_params *params, int argc,
                          char **argv) {

    const struct option longopts[] = {{"device", required_argument, 0, 'd'},
                                      {"inodes", required_argument, 0, 'i'},
                                      {"size", required_argument, 0, 's'},
                                      {"create", no_argument, 0, 'c'},
                                      {"help", no_argument, 0, 'h'},
                                      {"log-level", required_argument, 0, 'l'},
                                      {0, 0, 0, 0}};

    int opt;

    // reset the state of getopt_long_only
    optind = 0;

    while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
        switch (opt) {
        case 'c':
            params->create = 1;
            break;
        case 'd':
            params->device = strdup(optarg);
            break;
        case 'i':
            if (!ex_cli_parse_number("inodes", optarg,
                                     &params->number_of_inodes)) {
                return EX_MKFS_OPTION_PARSE_ERROR;
            }
            break;
        case 'l':
            if (!ex_logging_init(optarg, 1)) {
                return EX_MKFS_OPTION_PARSE_ERROR;
            }
            break;
        case 's':
            if (!ex_cli_parse_number("size", optarg, &params->device_size)) {
                return EX_MKFS_OPTION_PARSE_ERROR;
            }
            break;
        case 'h':
            ex_mkfs_show_help();
            return EX_MKFS_OPTION_HELP;
        default:
            ex_mkfs_show_help();
            return EX_MKFS_OPTION_UNKNOWN;
        }
    }

    return EX_MKFS_OPTION_OK;
}

void ex_mkfs_free_params(struct ex_mkfs_params *params) {
    free(params->device);
}
