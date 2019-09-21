#include "ex.h"
#include "errors.h"

size_t ex_device_size(size_t ninodes) {
    return ninodes * EX_DIRECT_BLOCKS *
               EX_BLOCK_SIZE +             // space for n-1 inode data
           ninodes * EX_DIRECT_BLOCKS +    // space for n-1 inodes
           sizeof(struct ex_super_block) + // space for superblock
           ninodes / 8 +                   // size of inode bitmap
           ninodes * EX_DIRECT_BLOCKS / 8; // size of data bitmap
}

ex_status ex_init(const char *device) {

    info("opening device: %s", device);

    ex_status status = OK;

    if ((status = ex_device_open(device)) != OK) {
        goto open_error;
    }

    ex_super_load();

    if ((status = ex_root_load()) != OK) {
        goto root_load_error;
    }

    return status;

root_load_error:
    ex_device_close();

open_error:

    error("unable to open device: %s", device);

    return status;
}

void ex_deinit(void) {

    info("deinitializing fs");

    if (ex_is_device_opened()) {
        ex_device_close();
    }

    if (super_block) {
        info("freeing super_block");
        free(super_block);
    }

    if (root) {
        info("freeing root");
        free(root);
    }
}

int ex_create(const char *pathname, mode_t mode, gid_t gid, uid_t uid) {

    ex_super_lock();

    int rv = 0;

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *dirpath = ex_path_make_dirpath(pathname);
    struct ex_path *path = ex_path_make(pathname);

    struct ex_inode *destdir = ex_inode_find(dirpath);

    if (!destdir) {
        rv = -ENOENT;
        goto free_destdir;
    }

    struct ex_inode inode;

    // we do not have enough space for a new inode
    if (ex_inode_create(&inode, mode, gid, uid) != OK) {
        rv = -ENOSPC;
        goto free_destdir;
    }

    // check if directory has enough space for a new inode
    if (!ex_inode_set(destdir, path->basename, &inode)) {
        ex_inode_deallocate_blocks(&inode);
        rv = -ENOSPC;
    }

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

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_path;
    }

    st->st_nlink = inode->nlinks;
    st->st_size = inode->size;
    st->st_mode = inode->mode;
    st->st_ino = inode->number;

    st->st_mtim = inode->mtime;
    st->st_atim = inode->atime;
    st->st_ctim = inode->ctime;

    st->st_uid = inode->uid;
    st->st_gid = inode->gid;

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

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *dirpath = ex_path_make_dirpath(pathname);
    struct ex_inode *dir = ex_inode_find(dirpath);

    if (!dir) {
        rv = -ENOENT;
        goto free_dir;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_unlink(dir, path->basename);

    if (!inode) {
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

    if (!ex_super_check_path_len(src_pathname) ||
        !ex_super_check_path_len(dest_pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *src_path = ex_path_make(src_pathname);
    struct ex_inode *src_inode = ex_inode_find(src_path);

    if (!src_inode) {
        rv = -ENOENT;
        goto free_src_inode;
    }

    if (src_inode->mode & S_IFDIR) {
        rv = -EPERM;
        goto free_src_inode;
    }

    if (src_inode->nlinks == USHRT_MAX) {
        rv = -EMLINK;
        goto free_src_inode;
    }

    struct ex_path *dest_dir_path = ex_path_make_dirpath(dest_pathname);
    struct ex_inode *dest_dir_inode = ex_inode_find(dest_dir_path);

    if (!dest_dir_inode) {
        rv = -ENOENT;
        goto free_dest_dir_inode;
    }

    if (!(dest_dir_inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto free_src_inode;
    }

    struct ex_path *dest_path = ex_path_make(dest_pathname);

    if (!ex_inode_set(dest_dir_inode, dest_path->basename, src_inode)) {
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

    if (!inode) {
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

    if (!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    rv = ex_inode_write(inode, offset, buf, size);

    if (!rv) {
        rv = -EIO;
        goto free_inode;
    }

    if (rv == -1) { // ENOSPC?
        rv = -EFBIG;
        goto free_inode;
    }

    ex_update_time_ns(&inode->mtime);
    ex_inode_flush(inode);

free_inode:
    ex_path_free(path);
    ex_inode_free(inode);

    ex_super_unlock();

    return rv;
}

int ex_open(const char *pathname, int flags, gid_t gid, uid_t uid) {
    // XXX: this function should do nothing if -odefault_permissions is
    //      specified on the command line
    ex_super_lock();

    info("path=%s, flags=%i, gid=%i, uid=%i", pathname, flags,
            gid, uid);

    ssize_t rv = 0;

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    int perm = 0;

    if (flags == O_RDONLY) {
        perm = EX_READ;
    } else if (flags == O_WRONLY) {
        perm = EX_WRITE;
    } else if (flags == O_RDWR) {
        perm = EX_WRITE | EX_READ;
#ifdef O_EXEC
    } else if (flags == O_EXEC) {
        perm = EX_EXECUTE;
#endif
    }
    // NOTE: we igore the other flags

    if (!ex_inode_has_perm(inode, perm, gid, uid)) {
        rv = -EPERM;
    }

free_inode:
    ex_path_free(path);
    ex_inode_free(inode);

    ex_super_unlock();

    return rv;
}

int ex_mkdir(const char *pathname, mode_t mode, gid_t gid, uid_t uid) {

    ex_super_lock();

    int rv = 0;

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *destpath = ex_path_make_dirpath(pathname);
    struct ex_inode *destdir = ex_inode_find(destpath);

    if (!destdir) {
        rv = -ENOENT;
        goto free_inode;
    }

    struct ex_path *dirpath = ex_path_make(pathname);
    struct ex_inode dir;

    // we do not have enough space for a new inode
    if (ex_inode_create(&dir, mode | S_IFDIR, gid, uid) != OK) {
        rv = -ENOSPC;
        goto free_all;
    }

    ex_inode_fill_dir(&dir, destdir);
    ex_inode_set(destdir, dirpath->basename, &dir);

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

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    if (inode->mode & S_IFDIR) {
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

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    if (!(inode->mode & S_IFDIR)) {
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

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
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

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_path;
    }

    if (!(inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto free_inode;
    }

    struct ex_path *dirpath = ex_path_make_dirpath(pathname);
    struct ex_inode *dir = ex_inode_find(dirpath);

    if (!dir) {
        rv = -ENOENT;
        goto free_dir_inode;
    }

    struct ex_inode *removed = ex_inode_unlink(dir, path->basename);

    if (!removed) {
        rv = -ENOTEMPTY;
    }

    ex_inode_free(removed);

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

int ex_chmod(const char *pathname, mode_t mode) {

    ex_super_lock();

    int rv = 0;

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    ex_log_mode(mode);
    ex_log_mode(inode->mode);

    // clean permissions mode
    inode->mode &= ~((2048 << 1) - 1);
    inode->mode |= mode;

    ex_log_mode(inode->mode);

    ex_inode_flush(inode);

free_inode:
    ex_path_free(path);
    ex_inode_free(inode);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_access(const char *pathname, int mode) {

    ex_super_lock();

    int rv = 0;

    if (!ex_super_check_path_len(pathname)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto free_inode;
    }

    if (mode == F_OK) {
        goto free_inode;
    }

    if (!(mode & inode->mode) && !(mode & (inode->mode >> 3)) &&
        !(mode & (inode->mode >> 6))) {
        rv = -EACCES;
    }

free_inode:
    ex_path_free(path);
    ex_inode_free(inode);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_symlink(const char *target, const char *link) {

    ex_super_lock();

    int rv = 0;

    if (!ex_super_check_path_len(target) || !ex_super_check_path_len(link)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    // check if target exists
    struct ex_path *target_path = ex_path_make(target);
    struct ex_inode *target_inode = ex_inode_find(target_path);

    if (!target_inode) {
        rv = -ENOENT;
        goto target_not_found;
    }

    // find directory where `link` will be placed
    struct ex_path *link_dir_path = ex_path_make_dirpath(link);
    struct ex_inode *link_dir_inode = ex_inode_find(link_dir_path);

    if (!link_dir_path) {
        rv = -ENOENT;
        goto link_dir_is_invalid;
    }

    if (!(link_dir_inode->mode & S_IFDIR)) {
        rv = -ENOTDIR;
        goto link_dir_is_invalid;
    }

    // check that `link` does not exist
    struct ex_path *link_path = ex_path_make(link);
    struct ex_inode *link_inode = ex_inode_find(link_path);

    if (link_inode) {
        rv = -EEXIST;
        goto link_exists;
    }

    // create link inode
    mode_t mode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
    link_inode = ex_malloc(sizeof(struct ex_inode));

    if (ex_inode_create(link_inode, mode, target_inode->gid, target_inode->uid) != OK) {
        rv = -ENOSPC;
        goto fail;
    }

    // write target path into inode data blocks
    size_t target_len = strlen(target);

    if (ex_inode_write(link_inode, 0, target, target_len) !=
        (ssize_t)target_len) {
        rv = -EIO;
        ex_inode_deallocate_blocks(link_inode);
        goto fail;
    }

    // add link inode into link directory
    if (!ex_inode_set(link_dir_inode, link_path->basename, link_inode)) {
        rv = -ENOSPC;
        ex_inode_deallocate_blocks(link_inode);
    }

fail:
link_exists:
    ex_path_free(link_path);
    if (link_inode)
        ex_inode_free(link_inode);

link_dir_is_invalid:
    ex_path_free(link_dir_path);
    ex_inode_free(link_dir_inode);

target_not_found:
    ex_path_free(target_path);
    ex_inode_free(target_inode);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_readlink(const char *pathname, char *buffer, size_t bufsize) {

    ex_super_lock();

    int rv = 0;

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto invalid_path;
    }

    size_t maxread = inode->size < bufsize ? inode->size : bufsize;

    ex_inode_read(inode, 0, buffer, maxread);
    ex_inode_free(inode);

invalid_path:
    ex_super_unlock();

    ex_path_free(path);
    return rv;
}

int ex_rename(const char *from, const char *to) {

    debug("from: %s, to: %s", from, to);

    ex_super_lock();

    int rv = 0;

    if (!ex_super_check_path_len(from) || !ex_super_check_path_len(to)) {
        rv = -ENAMETOOLONG;
        goto name_too_long;
    }

    // check if from exists
    struct ex_path *from_path_dir = ex_path_make_dirpath(from);
    struct ex_path *from_path = ex_path_make(from);
    struct ex_inode *from_inode_dir = ex_inode_find(from_path_dir);

    if (!from_inode_dir) {
        rv = -ENOENT;
        goto from_not_found;
    }

    // find directory where `to` will be placed
    struct ex_path *to_path_dir = ex_path_make_dirpath(to);
    struct ex_path *to_path = ex_path_make(to);
    struct ex_inode *to_inode_dir = ex_inode_find(to_path_dir);

    if (!to_inode_dir) {
        rv = -ENOENT;
        goto to_dir_is_invalid;
    }

    rv = ex_inode_rename(from_inode_dir, to_inode_dir, from_path->basename,
                         to_path->basename);

to_dir_is_invalid:
    ex_path_free(to_path_dir);
    ex_inode_free(to_inode_dir);

from_not_found:
    ex_path_free(from_path_dir);
    ex_inode_free(from_inode_dir);

name_too_long:
    ex_super_unlock();

    return rv;
}

int ex_chown(const char *pathname, uid_t uid, gid_t gid) {

    int rv = 0;

    ex_super_lock();

    struct ex_path *path = ex_path_make(pathname);
    struct ex_inode *inode = ex_inode_find(path);

    if (!inode) {
        rv = -ENOENT;
        goto invalid_path;
    }

    inode->uid = uid;
    inode->gid = gid;

    ex_inode_flush(inode);
    ex_inode_free(inode);

invalid_path:
    ex_super_unlock();

    ex_path_free(path);
    return rv;
}
