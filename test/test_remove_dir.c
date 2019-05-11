#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_remove_dir(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create directory
    int rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR);
    g_assert(!rv);

    // add file to directory
    rv = ex_create("/dir/file", S_IRWXU);
    g_assert(!rv);

    // check that we get -ENOTEMPTY
    rv = ex_rmdir("/dir");
    g_assert_cmpint(rv, ==, -ENOTEMPTY);

    // remove file
    rv = ex_unlink("/dir/file");
    g_assert(!rv);

    // check that we can remove empty directory
    rv = ex_rmdir("/dir");
    g_assert(!rv);

    struct stat st;

    // check that the directory doesn't exists
    rv = ex_getattr("/dir", &st);
    g_assert(rv);

    ex_deinit();
}
