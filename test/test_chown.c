#include "../src/ex.h"
#include "../src/mkfs.h"
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_chown(void) {
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    int rv = ex_chown("/fname", 1, 1);
    g_assert(rv == -ENOENT);

    rv = ex_create("/fname", 0, getgid(), getuid());
    g_assert(!rv);

    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!st.st_mode);

    rv = ex_chown("/fname", 1, 1);
    g_assert(!rv);

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert(st.st_uid == 1);
    g_assert(st.st_gid == 1);

    ex_deinit();
}
