#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_stat_block_number(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);

    // current file should be empty, but because we do preallocate
    // all direct blocks nblocks should be EX_DIRECT_BLOCKS
    g_assert_cmpint(st.st_blocks, ==, EX_DIRECT_BLOCKS);

    // TODO: write to indirect blocks and check allocated blocks
    ex_deinit();
}
