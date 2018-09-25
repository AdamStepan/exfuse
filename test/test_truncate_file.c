#include <sys/types.h>
#include <sys/stat.h>
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

    // write something to file
    rv = ex_write("/fname", "xxx", 3, 0);
    if(rv != 3) {
        rv = 1;
        printf("write size != 3");
    }

    // check file size > 0
    struct stat st;

    rv = ex_getattr("/fname", &st);

    if(rv) {
        printf("getattr");
        goto end;
    }

    if(st.st_size != 3) {
        rv = 1;
        printf("st.st_size != 3");
        goto end;
    }


    // truncate file
    rv = ex_truncate("/fname");

    if(rv) {
        printf("truncate");
        goto end;
    }

    // check file size == 0
    rv = ex_getattr("/fname", &st);

    if(rv) {
        printf("getattr1");
        goto end;
    }

    if(st.st_size != 0) {
        printf("st.st_size != 0");
        rv = 1;
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
