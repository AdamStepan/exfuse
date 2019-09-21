#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_name_too_long(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // check max file length
    struct statvfs statbuf;

    int rv = ex_statfs(&statbuf);
    g_assert(!rv);

    size_t namemax = statbuf.f_namemax;
    char *name = ex_malloc(namemax + 1);

    memset(name, 'x', namemax);

    rv = ex_create(name, S_IRWXU, getgid(), getuid());
    g_assert_cmpint(rv, ==, -ENAMETOOLONG);

    ex_deinit();
}
