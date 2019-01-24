#include <linux/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create file
    int rv = ex_create("/file", S_IRWXU);

    if(rv) {
        warnx("create file");
        goto end;
    }

    // check that we can getattribute for file
    struct stat st;
    rv = ex_getattr("/file", &st);

    if(rv) {
        warnx("getattr");
        goto end;
    }

    rv = ex_unlink("/file");

    if(rv) {
        warnx("unlink");
        goto end;
    }

    rv = ex_getattr("/file", &st);

    if(!rv) {
        warnx("getattr removed");
        goto end;
    } else {
        rv = 0;
    }

end:
    ex_deinit();

    return rv;
}
