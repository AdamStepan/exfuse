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

    // create new file
    int rv = ex_create("/fname", S_IRWXU | S_IFDIR);

    if(rv) {
        warnx("create");
        goto end;
    }

    // check file atributes
    struct stat st;

    rv = ex_getattr("/fname", &st);

    if(rv) {
        warnx("gettatr");
        goto end;
    }

    if(!(st.st_mode & S_IFDIR)) {
        rv = 1;
        warnx("st_mode");
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
