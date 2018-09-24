#include <ex.h>

size_t ex_device_size(size_t ninodes) {
    return ninodes * EX_DIRECT_BLOCKS * EX_BLOCK_SIZE + // space for n inodes
        sizeof(struct ex_super_block) +                 // space for superblock
        EX_DIRECT_BLOCKS / 8;                           // size of block bitmap
}

void ex_init(void) {

    size_t device_size = ex_device_size(256);
    int created = ex_device_create(EX_DEVICE, device_size);

    info("device created: %d", created);

    ex_device_open(EX_DEVICE);

    if(created) {
        ex_super_write(device_size);
        ex_root_write();
    } else {
        ex_super_load();
        ex_root_load();
    }
}

void ex_deinit(void) {

    info("deinitialize");

    if(device_fd != -1) {
        info("closing device: %i", device_fd);
        close(device_fd);
    }

    if(super_block) {
        info("freeing super_block");
        free(super_block);
    }

    if(root) {
        info("freeing root");
        free(root);
    }
}

void ex_print_struct_sizes(void) {
    info("ex_super_block: %lu", sizeof(struct ex_super_block));
    info("ex_inode: %lu", sizeof(struct ex_inode));
    info("ex_dir_entry: %lu", sizeof(struct ex_dir_entry));
}

int ex_create(const char *pathname, mode_t mode) {

    int rv = 0;

    struct ex_path *dirpath = ex_make_dir_path(pathname);
    struct ex_path *path = ex_make_path(pathname);

    struct ex_inode *destdir = ex_inode_find(root, dirpath);

    if(!destdir) {
        rv = -ENOENT;
        goto free_destdir;
    }

    struct ex_inode *inode = ex_inode_create(path->basename, mode);

    // TODO: we need to dealocate blocks if we cannot place inode to destdir
    ex_inode_set(destdir, inode);
    ex_inode_free(inode);

free_destdir:
    ex_inode_free(destdir);
    ex_free_path(path);
    ex_free_path(dirpath);

    return rv;
}

int ex_getattr(const char *pathname, struct stat *st) {

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_inode_find(root, path);

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
    ex_inode_free(inode);

    return 0;
}

int ex_unlink(const char *pathname) {

    int rv = 0;

    struct ex_path *dirpath = ex_make_dir_path(pathname);
    struct ex_inode *dir = ex_inode_find(root, dirpath);

    if(!dir) {
        rv = -ENOENT;
        goto free_dir;
    }

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_inode_remove(dir, path->basename);

    if(!inode) {
        rv = -ENOENT;
        goto free_all;
    }

free_all:
    ex_free_path(path);
    ex_inode_free(inode);

free_dir:
    ex_free_path(dirpath);
    ex_inode_free(dir);

    return rv;
}

int ex_read(const char *pathname, char *buffer, size_t size, off_t offset) {

    info("path=%s, offset=%lu, size=%lu", pathname, offset, size);

    int rv = 0;

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_inode_find(root, path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    char *data = ex_inode_read(inode, offset, size);
    rv = strlen(data);

    memcpy(buffer, data, rv);

    free(data);

free_inode:
    ex_inode_free(inode);
    ex_free_path(path);

    return rv;
}

int ex_write(const char *pathname, const char *buf, size_t size, off_t offset) {

    info("path=%s, off=%jd, size=%lu", pathname, offset, size);

    int rv = 0;

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_inode_find(root, path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    rv = ex_inode_write(inode, offset, buf, size);

free_inode:
    free(path);
    ex_inode_free(inode);

    return rv;
}

int ex_open(const char *pathname) {
    (void)pathname;
    return 0;
}

int ex_mkdir(const char *pathname, mode_t mode) {

    int rv = 0;

    struct ex_path *destpath = ex_make_dir_path(pathname);
    struct ex_inode *destdir = ex_inode_find(root, destpath);

    if(!destdir) {
        rv = -ENOENT;
        goto free_inode;
    }

    struct ex_path *dirpath = ex_make_path(pathname);
    struct ex_inode *dir = ex_inode_create(dirpath->basename, mode | S_IFDIR);

    ex_inode_set(destdir, dir);

    ex_free_path(dirpath);
    ex_inode_free(dir);

free_inode:
    ex_inode_free(destdir);
    ex_free_path(destpath);

    return rv;
}

int ex_readdir(const char *pathname, struct ex_inode ***inodes) {

    int rv = 0;

    struct ex_path *path = ex_make_path(pathname);
    struct ex_inode *inode = ex_inode_find(root, path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    if(!(inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto free_inode;
    }

    *inodes = ex_inode_get_all(inode);

free_inode:
    ex_inode_free(inode);
    ex_free_path(path);

    return rv;
}

int ex_utimens(const char *pathname, const struct timespec tv[2]) {
    (void)pathname;
    (void)tv;

    // XXX: implement utimensat (2)
    return 0;
}
