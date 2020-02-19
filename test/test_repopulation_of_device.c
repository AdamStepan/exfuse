#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>

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

static int clean_device(size_t ninodes) {

    int rv = 0;
    char buffer[16];

    for (size_t i = 0; i < ninodes - 1; i++) {

        snprintf(buffer, sizeof(buffer), "/file%zu", i);

        rv = ex_unlink(buffer);

        if (rv) {
            warnx("ex_create(%s)", buffer);
            goto end;
        }
    }
end:
    return rv;
}

void test_bitmap_flip(void) {

    unlink("exdev");

    const size_t ninodes = 16;

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

    // we shouldn't be able to create the inode
    rv = ex_create("/shouldtwork", S_IRWXU, getgid(), getuid());
    g_assert_cmpint(rv, ==, -ENOSPC);

    // remove the first file and try to create a new one
    rv = ex_unlink("/file0");
    g_assert(!rv);

    struct stat buf;
    rv = ex_getattr("/file0", &buf);
    g_assert_cmpint(rv, ==, -ENOENT);

    rv = ex_create("/shouldwork", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    rv = ex_getattr("/shouldwork", &buf);
    g_assert(!rv);
}

void test_repopulation_of_device(void) {

    unlink("exdev");

    const size_t ninodes = 256;

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

    rv = clean_device(ninodes);
    g_assert(!rv);

    rv = populate_device(ninodes);
    g_assert(!rv);

    ex_deinit();
}
