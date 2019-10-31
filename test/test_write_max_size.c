#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_write_max_size(void) {
    // create new device
    unlink(EX_DEVICE);

    ex_mkfs_test_init();

    size_t max_size = ex_inode_max_blocks() * EX_BLOCK_SIZE - 1;
    // +1 because of EFBIG
    char *data = malloc(max_size + 1);
    g_assert(data);

    memset(data, 'a', max_size + 1);

    // create new file
    int rv = ex_create("/file", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    rv = ex_write("/file", data, max_size, 0);
    g_assert_cmpint((size_t)rv, ==, max_size);

    rv = ex_write("/file", data, max_size + 1, 0);
    g_assert_cmpint(rv, ==, -EFBIG);

    rv = ex_write("/file", data, 0, max_size + 1);
    g_assert_cmpint(rv, ==, -EFBIG);

    ex_deinit();
}
