#define FUSE_USE_VERSION 30

#include <errno.h>
#include <ex.h>
#include <fuse.h>
#include <libgen.h>
#include <linux/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

struct ex_args {
    char *loglevel;
    char *device;
};

static int do_create(const char *pathname, mode_t mode,
                     struct fuse_file_info *fi) {
    (void)fi;
    return ex_create(pathname, mode);
}

static int do_getattr(const char *pathname, struct stat *st) {
    return ex_getattr(pathname, st);
}

static int do_mkdir(const char *pathname, mode_t mode) {
    return ex_mkdir(pathname, mode);
}

static int do_open(const char *pathname, struct fuse_file_info *fi) {
    (void)fi;
    return ex_open(pathname);
}

static int do_readdir(const char *pathname, void *buffer,
                      fuse_fill_dir_t filler, off_t offset,
                      struct fuse_file_info *fi) {
    (void)fi;

    assert(!offset);

    struct ex_dir_entry **entries;
    int rv = ex_readdir(pathname, &entries);

    if (rv) {
        goto free_entries;
    }

    for (size_t i = 0; entries[i]; i++) {
        debug("inode name=%s", entries[i]->name);
        filler(buffer, entries[i]->name, NULL, 0);
    }

free_entries:

    for (size_t i = 0; entries[i]; i++) {
        ex_dir_entry_free(entries[i]);
    }

    free(entries);

    return rv;
}

static int do_read(const char *pathname, char *buffer, size_t size,
                   off_t offset, struct fuse_file_info *fi) {
    (void)fi;

    return ex_read(pathname, buffer, size, offset);
}

static int do_truncate(const char *pathname, off_t off) {

    (void)off;

    return ex_truncate(pathname);
}

static int do_write(const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    (void)fi;

    return ex_write(path, buf, size, offset);
}

static int do_unlink(const char *pathname) { return ex_unlink(pathname); }

static int do_utimens(const char *pathname, const struct timespec tv[2]) {
    return ex_utimens(pathname, tv);
}

static int do_link(const char *srcpath, const char *destpath) {
    return ex_link(srcpath, destpath);
}

static int do_rmdir(const char *pathname) { return ex_rmdir(pathname); }

static int do_statfs(const char *pathname, struct statvfs *statbuffer) {
    (void)pathname;
    return ex_statfs(statbuffer);
}

static void *do_init(struct fuse_conn_info *conn) {
    (void)conn;

    struct fuse_context *ctx = fuse_get_context();

    if (!ctx) {
        fatal("unable to get fuse context");
    }

    struct ex_args *args = (struct ex_args *)ctx->private_data;



    ex_logging_init(args->loglevel);
    ex_init(args->device);

    return NULL;
}

static void do_destroy(void *private_data) {
    (void)private_data;

    ex_deinit();
}

static int do_chmod(const char *pathname, mode_t mode) {
    return ex_chmod(pathname, mode);
}

static int do_access(const char *pathname, int mode) {
    return ex_access(pathname, mode);
}

static int do_symlink(const char *target, const char *link) {
    return ex_symlink(target, link);
}

static int do_readlink(const char *link, char *buffer, size_t bufsize) {
    return ex_readlink(link, buffer, bufsize);
}

static int do_rename(const char *from, const char *to) {
    return ex_rename(from, to);
}

static struct fuse_operations operations = {.getattr = do_getattr,
                                            .readdir = do_readdir,
                                            .read = do_read,
                                            .create = do_create,
                                            .open = do_open,
                                            .write = do_write,
                                            .unlink = do_unlink,
                                            .mkdir = do_mkdir,
                                            .utimens = do_utimens,
                                            .init = do_init,
                                            .destroy = do_destroy,
                                            .truncate = do_truncate,
                                            .link = do_link,
                                            .rmdir = do_rmdir,
                                            .statfs = do_statfs,
                                            .chmod = do_chmod,
                                            .access = do_access,
                                            .symlink = do_symlink,
                                            .readlink = do_readlink,
                                            .rename = do_rename};

static void ex_args_init(struct ex_args *args) {
    args->loglevel = NULL;
    args->device = NULL;
}

static void ex_args_finalize(struct ex_args *args) {

    if (!args->device) {
        fatal("device was not specified");
    }

    char *absolute_path = ex_malloc(PATH_MAX);

    if (!realpath(args->device, absolute_path)) {
        err(errno, "realpath: %s", args->device);
    }

    free(args->device);
    args->device = absolute_path;
}

static struct fuse_opt ex_opts[] = {
    {"--log-level %s", offsetof(struct ex_args, loglevel), FUSE_OPT_KEY_OPT},
    {"--device %s", offsetof(struct ex_args, device), FUSE_OPT_KEY_OPT},
    {NULL, 0, 0}};

int main(int argc, char **argv) {

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct ex_args exargs;

    ex_args_init(&exargs);
    fuse_opt_parse(&args, &exargs, ex_opts, NULL);
    ex_args_finalize(&exargs);

    return fuse_main(args.argc, args.argv, &operations, &exargs);
}
