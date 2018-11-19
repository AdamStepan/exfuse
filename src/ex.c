#include <ex.h>

size_t ex_device_size(size_t ninodes) {
    return ninodes * EX_DIRECT_BLOCKS * EX_BLOCK_SIZE + // space for n-1 inode data
        ninodes * EX_DIRECT_BLOCKS +                    // space for n-1 inodes
        sizeof(struct ex_super_block) +                 // space for superblock
        ninodes / 8 +                                   // size of inode bitmap
        ninodes * EX_DIRECT_BLOCKS / 8;                 // size of data bitmap
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

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *dirpath = ex_path_make_dirpath(pathname);
    struct ex_path *path = ex_path_make(pathname);

    struct ex_inode *destdir = ex_inode_find(dirpath);

    if(!destdir) {
        rv = -ENOENT;
        goto free_destdir;
    }

    struct ex_inode *inode = ex_inode_create(mode);
    // we do not have enough space for a new inode
    if(!inode) {
        rv = -ENOSPC;
        goto free_destdir;
    }

    // TODO: we need to dealocate blocks if we cannot place inode to destdir
    ex_inode_set(destdir, path->basename, inode);
    ex_inode_free(inode);

free_destdir:
    ex_inode_free(destdir);
    ex_path_free(path);
    ex_path_free(dirpath);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_getattr(const char *pathname, struct stat *st) {

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_path;
    }

    st->st_nlink = inode->nlinks;
    st->st_size = inode->size;
    st->st_mode = inode->mode;

    st->st_mtim = inode->mtime;
    st->st_atim = inode->atime;
    st->st_ctim = inode->ctime;

    st->st_uid = getuid();
    st->st_gid = getgid();

    // update access time
    ex_update_time_ns(&inode->atime);
    ex_inode_flush(inode);

    ex_inode_free(inode);

free_path:
    ex_path_free(path);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_unlink(const char *pathname) {

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *dirpath = ex_path_make_dirpath(pathname);
    struct ex_inode *dir = ex_inode_find(dirpath);

    if(!dir) {
        rv = -ENOENT;
        goto free_dir;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_unlink(dir, path->basename);

    if(!inode) {
        rv = -ENOENT;
        goto free_all;
    }

free_all:
    ex_path_free(path);
    ex_inode_free(inode);

free_dir:
    ex_path_free(dirpath);
    ex_inode_free(dir);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_link(const char *src_pathname, const char *dest_pathname) {

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(src_pathname) ||
       !ex_super_check_path_len(dest_pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *src_path = ex_path_make(src_pathname);
    struct ex_inode *src_inode = ex_inode_find(src_path);

    if(!src_inode) {
        rv = -ENOENT;
        goto free_src_inode;
    }

    if(src_inode->mode & S_IFDIR) {
        rv = -EPERM;
        goto free_src_inode;
    }

    if(src_inode->nlinks == USHRT_MAX) {
        rv = -EMLINK;
        goto free_src_inode;
    }

    struct ex_path *dest_dir_path = ex_path_make_dirpath(dest_pathname);
    struct ex_inode *dest_dir_inode = ex_inode_find(dest_dir_path);

    if(!dest_dir_inode) {
        rv = -ENOENT;
        goto free_dest_dir_inode;
    }

    if(!(dest_dir_inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto free_src_inode;
    }

    struct ex_path *dest_path = ex_path_make(dest_pathname);

    if(!ex_inode_set(dest_dir_inode, dest_path->basename, src_inode)) {
        rv = -ENOSPC;
    }

    src_inode->nlinks += 1;
    ex_inode_flush(src_inode);

    ex_path_free(dest_path);

free_dest_dir_inode:
    ex_inode_free(dest_dir_inode);
    ex_path_free(dest_dir_path);

free_src_inode:
    ex_inode_free(src_inode);
    ex_path_free(src_path);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_read(const char *pathname, char *buffer, size_t size, off_t offset) {

    ex_super_lock();

    info("path=%s, offset=%lu, size=%lu", pathname, offset, size);

    int rv = 0;

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    rv = ex_inode_read(inode, offset, buffer, size);

    // update inode access time
    ex_update_time_ns(&inode->atime);
    ex_inode_flush(inode);

free_inode:
    ex_inode_free(inode);
    ex_path_free(path);

    info("read rv=%i", rv);

    ex_super_unlock();

    return rv;
}

int ex_write(const char *pathname, const char *buf, size_t size, off_t offset) {

    ex_super_lock();

    info("path=%s, off=%jd, size=%lu", pathname, offset, size);

    ssize_t rv = 0;

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    rv = ex_inode_write(inode, offset, buf, size);

    if(!rv) {
        rv = -EIO;
        goto free_inode;
    }

    if(rv == -1) { // ENOSPC?
        rv = -EFBIG;
        goto free_inode;
    }

    ex_update_time_ns(&inode->mtime);
    ex_inode_flush(inode);

free_inode:
    free(path);
    ex_inode_free(inode);

    ex_super_unlock();

    return rv;
}

int ex_open(const char *pathname) {
    (void)pathname;
    return 0;
}

int ex_mkdir(const char *pathname, mode_t mode) {

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *destpath = ex_path_make_dirpath(pathname);
    struct ex_inode *destdir = ex_inode_find(destpath);

    if(!destdir) {
        rv = -ENOENT;
        goto free_inode;
    }

    struct ex_path *dirpath = ex_path_make(pathname);
    struct ex_inode *dir = ex_inode_create(mode | S_IFDIR);

    // we do not have enough space for a new inode
    if(!dir) {
        rv = -ENOSPC;
        goto free_all;
    }

    ex_inode_fill_dir(dir);
    ex_inode_set(destdir, dirpath->basename, dir);
    ex_inode_free(dir);

free_all:
    ex_path_free(dirpath);

free_inode:
    ex_inode_free(destdir);
    ex_path_free(destpath);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_truncate(const char *pathname) {
    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    if(inode->mode & S_IFDIR) {
        rv = -EISDIR;
        goto free_inode;
    }

    // set inode size to 0 and update access/modification time
    inode->size = 0;
    ex_update_time_ns(&inode->mtime);
    inode->ctime = inode->mtime;

    ex_inode_flush(inode);

free_inode:
    ex_inode_free(inode);
    ex_path_free(path);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_readdir(const char *pathname, struct ex_dir_entry ***entries) {

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }


    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    if(!(inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto free_inode;
    }

    *entries = ex_inode_get_all(inode);

    // update inode access time
    ex_update_time_ns(&inode->atime);
    ex_inode_flush(inode);

    // update inode access time
    ex_update_time_ns(&inode->atime);
    ex_inode_flush(inode);

free_inode:
    ex_inode_free(inode);
    ex_path_free(path);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_utimens(const char *pathname, const struct timespec tv[2]) {

    ex_super_lock();

    #define ATIM 0
    #define MTIM 1

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    inode->atime = tv[ATIM];
    inode->mtime = tv[MTIM];

    ex_inode_flush(inode);

free_inode:
    ex_inode_free(inode);
    ex_path_free(path);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_rmdir(const char *pathname) {

    ex_super_lock();

    int rv = 0;

    if(!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if(!inode) {
        rv = -ENOENT;
        goto free_path;
    }

    if(!(inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto free_inode;
    }

    struct ex_path *dirpath = ex_path_make_dirpath(pathname);
    struct ex_inode *dir = ex_inode_find(dirpath);

    if(!dir) {
        rv =  -ENOENT;
        goto free_dir_inode;
    }

    if(!ex_inode_unlink(dir, path->basename)) {
        rv = -ENOTEMPTY;
    }

free_dir_inode:
    ex_path_free(dirpath);
    ex_inode_free(dir);

free_path:
    ex_path_free(path);

free_inode:
    ex_inode_free(inode);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_statfs(struct statvfs *statbuf) {

    ex_super_lock();

    ex_super_statfs(statbuf);
    ex_super_print(super_block);

    ex_super_unlock();

    return 0;
}
