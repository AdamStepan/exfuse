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

    char buffer[16];

    for (size_t i = 0; i < ninodes - 1; i++) {

        snprintf(buffer, sizeof(buffer), "/file%zu", i);

        rv = ex_create(buffer, S_IRWXU);

        if(rv) {
            warnx("ex_create(%s)", buffer);
            goto end;
        }
    }

    for (size_t i = 0; i < ninodes - 1; i++) {

        snprintf(buffer, sizeof(buffer), "/file%zu", i);

        struct stat buf;

        rv = ex_getattr(buffer, &buf);

        if(rv) {
            warnx("ex_getattr(%s)", buffer);
            goto end;
        }
    }
end:
    return rv;
}
