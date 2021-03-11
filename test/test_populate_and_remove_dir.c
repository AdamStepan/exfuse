#include "../src/ex.h"
#include "../src/mkfs.h"
#include "../src/device.h"

#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

void test_populate_and_remove_dir(void) {

    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new directory
    int rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR, getgid(), getuid());
    g_assert(!rv);

    char buffer[16];

    // check that we can create files in a directory
    for (int i = 0; i < 16; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        rv = ex_create(buffer, S_IRWXU, getgid(), getuid());
        g_assert(!rv);
    }

    // check that we can query files attributes
    for (int i = 0; i < 16; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        struct stat st;
        rv = ex_getattr(buffer, &st);
        g_assert(!rv);
    }

    // check that we cannot remove the populated directory
    rv = ex_rmdir("/dir");
    g_assert(rv);

    // check that we are able to delete all files
    for (int i = 0; i < 16; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        rv = ex_unlink(buffer);
        g_assert(!rv);
    }

    // check that we can remove the populated directory
    rv = ex_rmdir("/dir");
    g_assert(!rv);

    ex_deinit();
}
