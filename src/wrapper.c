#define FUSE_USE_VERSION 30

#include <linux/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <libgen.h>
#include <fuse.h>
#include <ex.h>

static int do_create(const char *pathname, mode_t mode, struct fuse_file_info *fi) {
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

static int do_readdir(const char *pathname, void *buffer, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi) {
    (void)fi;

    assert(!offset);

    struct ex_inode **inodes;
    int rv = ex_readdir(pathname, &inodes);

    if(rv) {
        goto free_inodes;
    }

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    for(size_t i = 0; inodes[i]; i++) {
        debug("inode name=%s", inodes[i]->name);
        filler(buffer, inodes[i]->name, NULL, 0);
    }

free_inodes:

    for(size_t i = 0; inodes[i]; i++) {
        ex_inode_free(inodes[i]);
    }

    free(inodes);

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

static int do_unlink(const char *pathname) {
    return ex_unlink(pathname);
}

static int do_utimens(const char *pathname, const struct timespec tv[2]) {
    return ex_utimens(pathname, tv);
}

static void* do_init(struct fuse_conn_info *conn) {
    (void)conn;

    ex_init();

    return NULL;
}

static void do_destroy(void *private_data) {
    (void)private_data;

    ex_deinit();
}

static struct fuse_operations operations = {
    .getattr=do_getattr,
    .readdir=do_readdir,
    .read=do_read,
    .create=do_create,
    .open=do_open,
    .write=do_write,
    .unlink=do_unlink,
    .mkdir=do_mkdir,
    .utimens=do_utimens,
    .init=do_init,
    .destroy=do_destroy,
    .truncate=do_truncate
};

int main(int argc, char **argv) {
    return fuse_main(argc, argv, &operations, NULL);
}
