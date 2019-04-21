#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <err.h>
#include <ex.h>
#include <mkfs.h>
#include <glib.h>

void test_stat_time_update(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/fname", S_IRWXU);
    g_assert(!rv);

    // save atime and mtime
    struct stat st;

    rv = ex_getattr("/fname", &st);
    g_assert(!rv);

    // truncate file (change mtime and ctime)
    rv = ex_truncate("/fname");
    g_assert(!rv);

    // check file size
    struct stat st1;

    rv = ex_getattr("/fname", &st1);
    g_assert(!rv);
    g_assert_cmpint(st.st_mtim.tv_nsec, !=, st1.st_mtim.tv_nsec);

    ex_deinit();
}
