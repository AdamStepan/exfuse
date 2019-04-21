#include <ex.h>
#include <mkfs.h>
#include <err.h>
#include <glib.h>

void test_mkfs_device_size(void) {

    unlink("exdev");

    const size_t ninodes = 8;

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    params.number_of_inodes = ninodes;
    params.device = "exdev";
    params.create = 1;

    ex_mkfs_check_params(&params);

    int rv = ex_mkfs(&params);
    g_assert(!rv);


    ex_super_load();

    // the size of the super block an inode bitmap and a data bitmap rounded
    // to block size
    size_t expected_device_size = 3 * EX_BLOCK_SIZE;
    // size for `ninodes` inodes
    expected_device_size += EX_BLOCK_SIZE * ninodes;
    // number of data blocks for `ninodes` inodes
    expected_device_size += ex_inode_max_blocks() * ninodes * EX_BLOCK_SIZE;
    g_assert_cmpint(super_block->device_size, ==, expected_device_size);

}
