#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>
#include <mkfs.h>

int main(int argc, char **argv) {


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

    // check # of allocated blocks
    struct statvfs statbuf;

    rv = ex_statfs(&statbuf);

    if (rv) {
        warnx("ex_statvfs");
        goto end;
    }

    if (statbuf.f_files != ninodes) {
        warnx("ex_statbuf: invalid number of total inodes");
        rv = 1;
        goto end;
    }

    // NOTE: -1 is here becase of root
    if (statbuf.f_ffree != ninodes - 1) {
        warnx("ex_statbuf: invalid number of free inodes");
        rv = 2;
        goto end;
    }

    rv = ex_create("/file0", S_IRWXU);

    if (rv) {
        warnx("ex_create");
        goto end;
    }

    rv = ex_statfs(&statbuf);

    if (rv) {
        warnx("ex_statvfs1");
        goto end;
    }

    if (statbuf.f_ffree != ninodes - 2) {
        warnx("ex_statbuf: invalid number of free inodes");
        rv = 2;
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
