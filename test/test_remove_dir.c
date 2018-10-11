#include <linux/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create file
    int rv = ex_create("/dir", S_IRWXU | S_IFDIR);

    if(rv) {
        fprintf(stderr, "ex_create");
        goto end;
    }

    // add file to directory
    rv = ex_create("/dir/file", S_IRWXU);

    if(rv) {
        fprintf(stderr, "ex_create1");
        goto end;
    }

    // check that we get -ENOTEMPTY
    rv = ex_rmdir("/dir");

    if(rv != -ENOTEMPTY) {
        fprintf(stderr, "ex_rmdir (%d)", rv);
        rv = 1;
        goto end;
    }

    // remove file
    rv = ex_unlink("/dir/file");

    if(rv) {
        fprintf(stderr, "ex_unlink");
        goto end;
    }

    // check that we can remove empty directory
    rv = ex_rmdir("/dir");

    if(rv) {
        fprintf(stderr, "ex_rmdir");
        goto end;
    }

    struct stat st;

    rv = ex_getattr("/dir", &st);

    if(!rv) {
        rv = 1;
        fprintf(stderr, "ex_getattr1");
        goto end;
    } else {
        rv = 0;
    }

end:
    ex_deinit();

    return rv;
}
