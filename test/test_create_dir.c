#include <linux/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>
#include <mkfs.h>
#include <glib.h>

void test_create_dir(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU | S_IFDIR);
    g_assert(!rv);

    // check file atributes
    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert(st.st_mode & S_IFDIR);

    ex_deinit();
}
