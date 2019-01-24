#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU);

    if(rv) {
        warnx("create");
        goto end;
    }

    // save atime and mtime
    struct stat st;

    rv = ex_getattr("/fname", &st);

    if(rv) {
        warnx("getattr");
        goto end;
    }

    // truncate file (change mtime and ctime)
    rv = ex_truncate("/fname");

    if(rv) {
        warnx("truncate");
        goto end;
    }

    // check file size
    struct stat st1;

    rv = ex_getattr("/fname", &st1);

    if(rv) {
        warnx("getattr1");
        goto end;
    }

    if(st.st_mtim.tv_nsec == st1.st_mtim.tv_nsec) {
        warnx("st.st_mtim == st1.st_mtim");
        rv = 1;
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
