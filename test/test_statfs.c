#include <err.h>
#include <ex.h>
#include <glib.h>
#include <mkfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_statfs(void) {

    const size_t ninodes = 8;

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    params.number_of_inodes = ninodes;
    params.device = "exdev";
    params.create = 1;

    ex_mkfs_check_params(&params);

    int rv = ex_mkfs(&params);
    g_assert(!rv);

    // check # of allocated blocks
    struct statvfs statbuf;

    rv = ex_statfs(&statbuf);
    g_assert(!rv);
    g_assert_cmpint(statbuf.f_files, ==, ninodes);
    // NOTE: -1 is here becase of root
    g_assert_cmpint(statbuf.f_ffree, ==, ninodes - 1);

    rv = ex_create("/file0", S_IRWXU);
    g_assert(!rv);

    rv = ex_statfs(&statbuf);
    g_assert(!rv);
    g_assert_cmpint(statbuf.f_ffree, ==, ninodes - 2);

    ex_deinit();
}
