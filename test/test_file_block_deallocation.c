#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>
#include <mkfs.h>
#include <glib.h>

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
    rv = ex_create("/file", S_IRWXU);
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
