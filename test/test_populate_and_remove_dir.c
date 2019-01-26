#include <linux/stat.h>
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

    // create new directory
    int rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR);

    if(rv) {
        warnx("create /dir");
        goto end;
    }

    char buffer[16];

    // check that we can create files in a directory
    for (int i = 0; i < 16; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        rv = ex_create(buffer, S_IRWXU);

        if(rv) {
            warnx("ex_create(%s)", buffer);
            goto end;
        }
    }

    // check that we can query files attributes
    for (int i = 0; i < 16; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        struct stat st;
        rv = ex_getattr(buffer, &st);

        if(rv) {
            warnx("getattr");
            goto end;
        }
    }

    // check that we cannot remove the populated directory
    rv = ex_rmdir("/dir");

    if (!rv) {
        warnx("ex_rmdir removed non-empty directory");
        rv = 1;
        goto end;
    }

    // check that we are able to delete all files
    for (int i = 0; i < 16; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        rv = ex_unlink(buffer);

        if (rv) {
            warnx("ex_unlink(%s) was unable to unlink file (%i)", buffer, rv);
            rv = 1;
            goto end;
        }
    }

    // check that we can remove the populated directory
    rv = ex_rmdir("/dir");

    if (rv) {
        warnx("ex_rmdir did not remove an empty directory");
    }


end:

    ex_deinit();

    return rv;
}
