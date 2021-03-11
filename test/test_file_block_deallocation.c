#include "../src/ex.h"
#include "../src/mkfs.h"
#include "../src/device.h"

#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_file_block_deallocation(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // check # of allocated blocks
    struct statvfs statbuf;

    int rv = ex_statfs(&statbuf);
    g_assert(!rv);

    size_t allocated = statbuf.f_bfree;

    // create new file
    rv = ex_create("/file", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    // remove the file
    rv = ex_unlink("/file");
    g_assert(!rv);

    // check that number of allocated blocks is the same
    // as before a file creation
    rv = ex_statfs(&statbuf);
    g_assert(!rv);
    g_assert_cmpint(statbuf.f_bfree, ==, allocated);

    ex_deinit();
}
