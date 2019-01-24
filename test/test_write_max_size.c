#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    size_t max_size = EX_DIRECT_BLOCKS * EX_BLOCK_SIZE - 1;
    char data[max_size];

    memset(data, 'a', sizeof(data));

    // create new file
    int rv = ex_create("/file", S_IRWXU);

    if(rv) {
        warnx("ex_create");
        goto end;
    }

    rv = ex_write("/file", data, max_size, 0);

    if((size_t)rv != max_size) {
        warnx("ex_write: written: %i", rv);
        goto end;
    }

    rv = ex_write("/file", data, max_size + 1, 0);

    if(rv != -EFBIG) {
        warnx("ex_write1: maximum file size exceeded: written: %i", rv);
        goto end;
    }

    rv = ex_write("/file", data, 0, max_size + 1);

    if(rv != -EFBIG) {
        warnx("ex_write2: maximum file size exceeded: written: %i", rv);
        goto end;
    }

    rv = 0;

end:
    ex_deinit();

    return 0;
}
