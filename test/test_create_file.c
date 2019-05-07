#include <ex.h>
#include <glib.h>
#include <mkfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_create_file(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU);
    g_assert(!rv);

    // check file atributes
    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);

    ex_deinit();
}
