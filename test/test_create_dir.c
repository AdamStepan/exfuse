#include <linux/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU | S_IFDIR);

    if(rv) {
        printf("create");
        goto end;
    }

    // check file atributes
    struct stat st;

    rv = ex_getattr("/fname", &st);

    if(rv) {
        printf("gettatr");
        goto end;
    }

    if(!(st.st_mode & S_IFDIR)) {
        rv = 1;
        printf("st_mode");
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
