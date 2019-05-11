#include "../src/ex.h"
#include "../src/mkfs.h"
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_chmod(void) {
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    int rv = ex_chmod("/fname", 0);
    g_assert(rv == -ENOENT);

    rv = ex_create("/fname", 0);
    g_assert(!rv);

    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!st.st_mode);

    rv = ex_chmod("/fname", S_ISVTX);
    g_assert(!rv);

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert(st.st_mode == S_ISVTX);

    ex_deinit();
}
