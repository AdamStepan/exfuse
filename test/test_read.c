#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_simple_read(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    char data[8];
    memset(data, 'a', sizeof(data));

    int rv = ex_create("/file", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    char buffer[sizeof(data)/sizeof(char)];

    rv = ex_write("/file", data, sizeof(data), 0);
    g_assert_cmpint(rv, ==, sizeof(buffer));

    rv = ex_read("/file", buffer, sizeof(buffer), 0);
    g_assert_cmpint(rv, ==, sizeof(buffer));

    rv = memcmp(data, buffer, sizeof(data));
    g_assert_cmpint(rv, ==, 0);

    ex_deinit();
}

void test_read_with_invalid_args(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    char data[8];
    memset(data, 'a', sizeof(data));

    int rv = ex_create("/file", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    char buffer[sizeof(data)/sizeof(char)];

    rv = ex_read("/file", buffer, sizeof(buffer), -1);
    g_assert_cmpint(rv, ==, EOF);

    rv = ex_read("/file", buffer, sizeof(buffer), ex_inode_max_blocks() * 2);
    g_assert_cmpint(rv, ==, EOF);

    rv = ex_read("/file", buffer, sizeof(buffer), 1);
    g_assert_cmpint(rv, ==, EOF);

    ex_deinit();
}

void test_read_no_entry(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    char buffer[128];
    int rv = ex_read("/file1", buffer, sizeof(buffer), 0);
    g_assert_cmpint(rv, ==, -ENOENT);

    ex_deinit();
}
