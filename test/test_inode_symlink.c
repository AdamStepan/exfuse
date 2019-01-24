#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>

int main(int argc, char **argv) {

    (void)argc;
    (void)argv;

    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create new file
    int rv = ex_create("/target", S_IRWXU);

    if(rv) {
        goto end;
    }

    rv = ex_symlink("/target", "/link");

    if(rv) {
        warnx("ex_symlink");
        goto end;
    }

    char buffer[512];

    // check that symlink now points to '/target'
    rv = ex_readlink("/target", buffer, sizeof(buffer));

    if(rv) {
        warnx("ex_readlink");
        goto end;
    }

    if(!strcmp("/target", buffer)) {
        warnx("target: %s instead of '/target'", buffer);
        rv = 1;
        goto end;
    }

    struct stat statbuf;

    rv = ex_getattr("/link", &statbuf);

    if(!rv) {
        warnx("ex_getattr");
        goto end;
    }

    if(!(statbuf.st_mode & S_IFLNK)) {
        warnx("st_mode & S_IFLINK == 0");
        rv = 2;
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
