#include <ex.h>
#include <mkfs.h>
#include <err.h>

int main(int argc, char **argv) {

    unlink("exdev");

    const size_t ninodes = 8;

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    params.number_of_inodes = ninodes;
    params.device = "exdev";
    params.create = 1;

    ex_mkfs_check_params(&params);

    int rv = ex_mkfs(&params);

    if (rv) {
        warnx("ex_mkfs: unable to create fs");
        goto end;
    }

    ex_super_load();

    // the size of the super block an inode bitmap and a data bitmap rounded
    // to block size
    size_t expected_device_size = 3 * EX_BLOCK_SIZE;
    // size for `ninodes` inodes
    expected_device_size += EX_BLOCK_SIZE * ninodes;
    // number of data blocks for `ninodes` inodes
    expected_device_size += ex_inode_max_blocks() * ninodes * EX_BLOCK_SIZE;

    if (super_block->device_size != expected_device_size) {
        rv = 1;
        warnx("ex_super_load: invalid device size, expected: %zu, real: %zu",
              expected_device_size, super_block->device_size);
        goto end;
    }

end:
    return rv;
}
