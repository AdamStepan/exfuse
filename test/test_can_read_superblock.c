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

    if (super_block->magic != EX_SUPER_MAGIC) {
        rv = 1;
        warnx("ex_super_load: invalid super block magic");
        goto end;
    }

    if (!super_block->root) {
        rv = 2;
        warnx("ex_super_load: root address is not set");
        goto end;
    }

    if (super_block->inode_bitmap.max_items != ninodes) {
        rv = 3;
        warnx("ex_super_load: inode bitmap has invalid number of items");
        goto end;
    }

end:
    return rv;
}
