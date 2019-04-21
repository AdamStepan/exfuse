#include <ex.h>
#include <mkfs.h>
#include <err.h>
#include <glib.h>

void test_can_read_superblock(void) {

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
    g_assert_cmpint(super_block->magic, ==, EX_SUPER_MAGIC);
    g_assert(super_block->root);
    g_assert_cmpint(super_block->inode_bitmap.max_items, ==, ninodes);

}
