#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU);

    if(rv) {
        printf("create");
        goto end;
    }

    // save atime and mtime
    struct stat st;

    rv = ex_getattr("/fname", &st);

    if(rv) {
        printf("getattr");
        goto end;
    }

    // truncate file (change mtime and ctime)
    rv = ex_truncate("/fname");

    if(rv) {
        printf("truncate");
        goto end;
    }

    // check file size
    struct stat st1;

    rv = ex_getattr("/fname", &st1);

    if(rv) {
        printf("getattr1");
        goto end;
    }

    if(st.st_mtim.tv_nsec == st1.st_mtim.tv_nsec) {
        printf("st.st_mtim == st1.st_mtim");
        rv = 1;
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
