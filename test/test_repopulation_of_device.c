#include <err.h>
#include <ex.h>
#include <glib.h>
#include <mkfs.h>

int populate_device(size_t ninodes) {

    int rv = 0;
    char buffer[16];

    for (size_t i = 0; i < ninodes - 1; i++) {

        snprintf(buffer, sizeof(buffer), "/file%zu", i);

        rv = ex_create(buffer, S_IRWXU);

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

int clean_device(size_t ninodes) {

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

    ex_super_load();

    rv = populate_device(ninodes);
    g_assert(!rv);

    rv = clean_device(ninodes);
    g_assert(!rv);

    rv = populate_device(ninodes);
    g_assert(!rv);
}
