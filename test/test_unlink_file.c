#include "../src/ex.h"
#include "../src/mkfs.h"
#include "../src/device.h"

#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_unlink_file(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create file
    int rv = ex_create("/file", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    // check that we can getattribute for file
    struct stat st;
    rv = ex_getattr("/file", &st);
    g_assert(!rv);

    rv = ex_unlink("/file");
    g_assert(!rv);

    rv = ex_getattr("/file", &st);
    g_assert(rv);

    ex_deinit();
}
