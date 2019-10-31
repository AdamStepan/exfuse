#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_dir_block_reallocation(void) {

    // create new device, maximum number of inodes must be greater
    // than EX_DIRECT_BLOCKS * (EX_BLOCK_SIZE / sizeof(struct ex_dir_entry))
    // because we want to check that directory is able to use indirect
    // blocks
    unlink("exdev");
    ex_set_log_level(debug);

    const size_t entry_per_block = EX_BLOCK_SIZE / sizeof(struct ex_dir_entry);
    const size_t ninodes = 2 * EX_DIRECT_BLOCKS * entry_per_block;

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    params.number_of_inodes = ninodes + 8; // space for . and .., we cannot use
                                           // 2 because number must be divisible
                                           // by 8
    params.device = "exdev";
    params.create = 1;

    ex_mkfs_check_params(&params);

    int rv = ex_mkfs(&params);
    g_assert(!rv);

    // create new directory
    rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR, getgid(), getuid());
    g_assert(!rv);

    char buffer[32];

    // check that we can create files in a directory
    for (size_t i = 0; i < ninodes; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%zu", i);

        rv = ex_create(buffer, S_IRWXU, getgid(), getuid());
        g_assert(!rv);
    }

    // check that we can query files attributes
    for (size_t i = 0; i < ninodes; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%zu", i);

        struct stat st;
        rv = ex_getattr(buffer, &st);
        g_assert(!rv);
    }

    // check that we cannot remove the populated directory
    rv = ex_rmdir("/dir");
    g_assert(rv);

    // check that we are able to delete all files
    for (size_t i = 0; i < ninodes; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%zu", i);

        rv = ex_unlink(buffer);
        g_assert(!rv);
    }

    // check that we can remove the populated directory
    rv = ex_rmdir("/dir");
    g_assert(!rv);

    ex_deinit();
}
