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

    int rv = 0;

    struct ex_path *dirpath = ex_make_dir_path(pathname);
    struct ex_path *path = ex_make_path(pathname);

    struct ex_inode *destdir = ex_dir_find(root, dirpath);

    if(!destdir) {
        rv = -ENOENT;
        goto free_destdir;
    }

    struct ex_inode *inode = ex_inode_create(path->basename, mode);

    // TODO: we need to dealocate blocks if we cannot place inode to destdir
    ex_dir_set_inode(destdir, inode);
    ex_free_inode(inode);

free_destdir:
    ex_free_inode(destdir);
    ex_free_path(path);
    ex_free_path(dirpath);

    return rv;
}

static int do_getattr(const char *pathname, struct stat *st) {

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_dir_find(root, path);

    if(!inode) {
        ex_free_path(path);
        return -ENOENT;
    }

    st->st_nlink = inode->mode & S_IFDIR ? 2 : 1;
    st->st_size = inode->size;
    st->st_mode = inode->mode;
    st->st_mtime = inode->mtime;

    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = st->st_ctime = time(NULL);

    ex_free_path(path);
    ex_free_inode(inode);

    return 0;
}

static int do_unlink(const char *pathname) {

    int rv = 0;

    struct ex_path *dirpath = ex_make_dir_path(pathname);
    struct ex_inode *dir = ex_dir_find(root, dirpath);

    if(!dir) {
        rv = -ENOENT;
        goto free_dir;
    }

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_dir_remove_inode(dir, path->basename);

    if(!inode) {
        rv = -ENOENT;
        goto free_all;
    }

free_all:
    ex_free_path(path);
    ex_free_inode(inode);

free_dir:
    ex_free_path(dirpath);
    ex_free_inode(dir);

    return rv;
}

static int do_readdir(const char *pathname, void *buffer, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *_) {

    int rv = 0;

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_dir_find(root, path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    struct ex_inode **inodes = ex_dir_get_inodes(inode);
    assert(inodes);

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    for(size_t i = 0; inodes[i]; i++) {
        info("ino=%s", inodes[i]->name);
        filler(buffer, inodes[i]->name, NULL, 0);
        free(inodes[i]);
    }

    free(inodes);

free_inode:
    ex_free_inode(inode);
    ex_free_path(path);

    return rv;
}

static int do_read(const char *pathname, char *buffer, size_t size,
                   off_t offset, struct fuse_file_info *fi) {

    info("path=%s, offset=%lu, size=%lu", pathname, offset, size);

    int rv = 0;

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_dir_find(root, path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    char *data = ex_inode_read(inode, offset, size);
    rv = strlen(data);

    memcpy(buffer, data, rv);

    free(data);

free_inode:
    ex_free_inode(inode);
    ex_free_path(path);

    return rv;
}

static int do_write(const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *_) {

    info("path=%s, off=%jd, size=%lu", path, offset, size);

    char *name = basename((char *)path);
    struct ex_inode * inode = ex_dir_load_inode(root, name);

    if(!inode) {
        return -ENOENT;
    }

    ex_inode_write(inode, offset, buf, size);

    return size;
};

static int do_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int do_mkdir(const char *pathname, mode_t mode) {

    int rv = 0;

    struct ex_path *destpath = ex_make_dir_path(pathname);
    struct ex_inode *destdir = ex_dir_find(root, destpath);

    if(!destdir) {
        rv = -ENOENT;
        goto free_inode;
    }

    struct ex_path *dirpath = ex_make_path(pathname);
    struct ex_inode *dir = ex_inode_create(dirpath->basename, mode | S_IFDIR);

    ex_dir_set_inode(destdir, dir);

    ex_free_path(dirpath);
    ex_free_inode(dir);

free_inode:
    ex_free_inode(destdir);
    ex_free_path(destpath);

    return rv;
}

static int do_utimens(const char *pathname, const struct timespec tv[2]) {
    (void)pathname;
    (void)tv;

    // XXX: implement utimensat (2)
    return 0;
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
    .destroy=do_destroy
};


int main(int argc, char **argv) {
    return fuse_main(argc, argv, &operations, NULL);
}
