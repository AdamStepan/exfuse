#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
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
    rv = ex_truncate("/fname");
    g_assert(!rv);

    // check file size == 0
    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert_cmpint(st.st_size, ==, 0);

    ex_deinit();
}
