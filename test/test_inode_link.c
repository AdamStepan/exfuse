#include "../src/ex.h"
#include "../src/mkfs.h"
#include "../src/device.h"

#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int check_links(const char *name, const char *info, size_t expected) {

    struct stat st;

    int rv = ex_getattr(name, &st);

    if (rv) {
        goto end;
    }

    if (st.st_nlink != expected) {
        rv = 1;
        warnx("%s", info);
        goto end;
    }

end:
    return rv;
}

void test_inode_link(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU, getgid(), getuid());
    g_assert(!rv);

    rv = check_links("/fname", "st_nlinks", 1);
    g_assert(!rv);

    rv = ex_link("/fname", "/link");
    g_assert(!rv);

    rv = check_links("/fname", "st_nlinks1", 2);
    g_assert(!rv);

    rv = check_links("/link", "st_nlinks2", 2);
    g_assert(!rv);

    ex_deinit();
}
