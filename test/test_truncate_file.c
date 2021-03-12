#include "../src/ex.h"
#include "../src/mkfs.h"
#include "../src/device.h"
#include "../src/inode.h"

#include <err.h>
#include <errno.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_truncate_file(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    // write something to file
    rv = ex_write("/fname", "xxx", 3, 0);
    g_assert_cmpint(rv, ==, 3);

    // check file size > 0
    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert_cmpint(st.st_size, ==, 3);

    // truncate file
    rv = ex_truncate("/fname", 0);
    g_assert(!rv);

    // check file size == 0
    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert_cmpint(st.st_size, ==, 0);

    rv = ex_truncate("/fname", 128);
    g_assert(!rv);

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert_cmpint(st.st_size, ==, 128);

    ex_deinit();
}

void test_truncate_invalid_arguments(void) {
    // create a new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create a new file
    int rv = ex_create("/fname", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    // write something to the file
    rv = ex_write("/fname", "xxx", 3, 0);
    g_assert_cmpint(rv, ==, 3);

    rv = ex_truncate("/fname", -1);
    g_assert_cmpint(rv, ==, -EINVAL);

    rv = ex_truncate("/", 0);
    g_assert_cmpint(rv, ==, -EISDIR);

    rv = ex_truncate("/fname", 2 * EX_BLOCK_SIZE * ex_inode_max_blocks());
    g_assert_cmpint(rv, ==, -EFBIG);

    ex_deinit();
}
