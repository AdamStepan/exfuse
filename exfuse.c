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

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    info("path=%s, mode=%#o", path, mode);

    char *name = basename((char *)path);
    struct ex_inode *inode = ex_inode_create(name, mode);

    ex_dir_set_inode(root, inode);

    free(inode);

    return 0;
}

static int do_getattr(const char *path, struct stat *st) {

    info("path=%s", path);

    struct ex_inode *inode = NULL;

    if(!strcmp(path, root->name)) {
        inode = root;
    } else {
        char *name = basename((char *)path);
        inode = ex_dir_load_inode(root, name);
    }

    if(!inode) {
        info("unknown inode: %s, returning ENONENT", path);
        return -ENOENT;
    }

    ex_print_inode(inode);

    st->st_nlink = inode->mode & S_IFDIR ? 2 : 1;
    st->st_size = inode->size;
    st->st_mode = inode->mode;
    st->st_mtime = inode->mtime;

    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = st->st_ctime = time(NULL);

    return 0;
}

static int do_unlink(const char *path) {

    info("path=%s", path);

    char *name = basename((char *)path);
    struct ex_inode *inode = ex_dir_remove_inode(root, name);

    if(!inode) {
        return -ENOENT;
    }

    info("removed");
    free(inode);

    return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *_) {

    info("path=%s", path);

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    if(strcmp(path, root->name)) {
        return 0;
    }

    struct ex_inode **inodes = ex_dir_get_inodes(root);
    assert(inodes);

    for(size_t i = 0; inodes[i]; i++) {
        filler(buffer, inodes[i]->name, NULL, 0);
        free(inodes[i]);
    }

    free(inodes);

    return 0;
}

static int do_read(const char *path, char *buffer, size_t size,
                   off_t offset, struct fuse_file_info *fi) {

    info("path=%s, offset=%lu, size=%lu", path, offset, size);

    char *name = basename((char *)path);
    struct ex_inode * inode = ex_dir_load_inode(root, name);
    if(!inode) {
        return -ENOENT;
    }

    char *data = ex_inode_read(inode, offset, size);
    size_t readed = strlen(data);

    memcpy(buffer, data, readed);

    free(data);

    return readed;
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

    info("path=%s, flags=%o", path, fi->flags);

    return 0;
}

void print_permissions(const char *prefix, uint8_t mode) {
    printf("%s=%c%c%c ", prefix,
        mode & 0x4 ? 'r' : '-',
        mode & 0x2 ? 'w' : '-',
        mode & 0x1 ? 'x' : '-');
}

void print_mode(mode_t m) {

    if(m & S_IFDIR)
        printf("dir");

    if(m & S_ISUID)
        printf("suid ");

    if(m & S_ISGID)
        printf("sgid ");

    if(m & S_ISVTX)
        printf("sticky ");

    print_permissions("other", m & 7);
    print_permissions("group", (m >> 3) & 7);
    print_permissions("user", (m >> 6) & 7);

    printf("\n");
}

static int do_mkdir(const char *name, mode_t mode) {
    print_mode(mode);
    info("name=%s, mode_t=%#o", name, mode);

    char *dname = basename((char *)name);

    struct ex_inode *dir = ex_inode_create(dname, mode | S_IFDIR);
    ex_dir_set_inode(root, dir);

    free(dir);

    return 0;
};

int do_utimens(const char *n, const struct timespec tv[2]) {
    // XXX: implement utimensat (2)
    return 0;
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
    .utimens=do_utimens
};

__attribute__((constructor))
static void init_fuse(void) { ex_init(); }

__attribute__((destructor))
static void deinit_fuse(void) { ex_deinit(); }

int main(int argc, char **argv) {
    return fuse_main(argc, argv, &operations, NULL);
}
