#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ex.h>

static int check_links(const char *name, const char *info, int expected) {

    struct stat st;

    int rv = ex_getattr(name, &st);

    if(rv) {
        goto end;
    }

    if(st.st_nlink != expected) {
        rv = 1;
        printf(info);
        goto end;
    }

end:
    return rv;
}

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU);

    if(rv) {
        goto end;
    }

    rv = check_links("/fname", "st_nlinks", 1);

    if(rv) {
        goto end;
    }

    rv = ex_link("/fname", "/link");

    if(rv) {
        printf("ex_link");
        goto end;
    }

    rv = check_links("/fname", "st_nlinks1", 2);

    if(rv) {
        goto end;
    }

    rv = check_links("/link", "st_nlinks2", 2);

    if(rv) {
        goto end;
    }

end:
    ex_deinit();

    return rv;
}
