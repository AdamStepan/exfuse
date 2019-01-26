#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>
#include <mkfs.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // check # of allocated blocks
    struct statvfs statbuf;

    int rv = ex_statfs(&statbuf);

    if(rv) {
        warnx("ex_statvfs");
        goto end;
    }

    size_t allocated = statbuf.f_bfree;

    // create new file
    rv = ex_create("/file", S_IRWXU);

    if(rv) {
        warnx("ex_create");
        goto end;
    }

    // remove the file
    rv = ex_unlink("/file");

    if(rv) {
        warnx("ex_unlink");
        goto end;
    }

    // check that number of allocated blocks is the same
    // as before a file creation
    rv = ex_statfs(&statbuf);

    if(rv) {
        warnx("ex_statvfs1");
        goto end;
    }

    if(statbuf.f_bfree != allocated) {
        warnx("statbuf.f_bfree != allocated");
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
