#include "../src/ex.h"
#include "../src/mkfs.h"
#include "../src/device.h"

#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_create_dir(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU | S_IFDIR, getgid(), getuid());
    g_assert(!rv);

    // check file atributes
    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);
    g_assert(st.st_mode & S_IFDIR);

    ex_deinit();
}
