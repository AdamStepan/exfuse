#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // check max file length
    struct statvfs statbuf;

    int rv = ex_statfs(&statbuf);

    if(rv) {
        printf("ex_statvfs");
        goto end;
    }

    size_t namemax = statbuf.f_namemax;
    char *name = ex_malloc(namemax + 1);

    memset(name, 'x', namemax + 1);

    rv = ex_create(name, S_IRWXU);

    if(rv != -ENAMETOOLONG) {
        printf("ex_create");
        rv = ENAMETOOLONG;
        goto end;
    } else {
        rv = 0;
    }

end:
    ex_deinit();

    return rv;
}
