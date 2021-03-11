#include "../src/ex.h"
#include "../src/mkfs.h"

#include <errno.h>
#include <err.h>
#include <glib.h>
#include <unistd.h>
#include <stdio.h>

static int populate_device(size_t ninodes) {

    int rv = 0;
    char buffer[16];

    for (size_t i = 0; i < ninodes - 1; i++) {

        snprintf(buffer, sizeof(buffer), "/file%zu", i);

        rv = ex_create(buffer, S_IRWXU, getgid(), getuid());

        if (rv) {
            warnx("ex_create(%s)", buffer);
            goto end;
        }
    }

    for (size_t i = 0; i < ninodes - 1; i++) {

        snprintf(buffer, sizeof(buffer), "/file%zu", i);

        struct stat buf;

        rv = ex_getattr(buffer, &buf);

        if (rv) {
            warnx("ex_getattr(%s)", buffer);
            goto end;
        }
    }
end:
    return rv;
}

void test_not_enough_space_for_inode() {

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

    rv = populate_device(ninodes);
    g_assert(!rv);

    rv = ex_create("/shouldnt-success", S_IRWXU, getgid(), getuid());
    g_assert_cmpint(rv, ==, -ENOSPC);

    ex_deinit();
}
