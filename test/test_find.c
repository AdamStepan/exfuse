#include "../src/ex.h"
#include "../src/mkfs.h"
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_inode_find(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    int rv = ex_mkdir("/a", S_IRWXU | S_IFDIR, getgid(), getuid());
    g_assert(!rv);

    rv = ex_mkdir("/a/b", S_IRWXU | S_IFDIR, getgid(), getuid());
    g_assert(!rv);

    rv = ex_create("/a/b/c", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    struct ex_path *path = ex_path_make("/a/b/c");
    struct ex_inode *inode = ex_inode_find(path);

    g_assert(inode);

    ex_path_free(path);
    ex_inode_free(inode);

    path = ex_path_make("/a/b");
    inode = ex_inode_find(path);

    g_assert(inode);

    ex_path_free(path);
    ex_inode_free(inode);

    path = ex_path_make("/");
    inode = ex_inode_find(path);

    g_assert(inode);

    ex_path_free(path);
    ex_inode_free(inode);

    path = ex_path_make("/a/b/c/d");
    inode = ex_inode_find(path);

    g_assert(!inode);

    ex_path_free(path);

    ex_deinit();
}
