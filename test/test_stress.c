#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_creation_and_deletion(void) {

    static const int N_OF_INODES = 1024;
    static char *DEVNAME = "test_creation_and_deletion";

    // create new device
    unlink(DEVNAME);

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    params.device = DEVNAME;
    params.create = 1;
    params.number_of_inodes = N_OF_INODES;

    ex_mkfs_check_params(&params);
    ex_mkfs(&params);

    // create new directory
    int rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR);
    g_assert(!rv);

    char buffer[32];

    // check that we can create files in a directory
    for (int i = 0; i < N_OF_INODES - 2; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        rv = ex_create(buffer, S_IRWXU);
        g_assert(!rv);
    }

    // check that we can query files attributes
    for (int i = 0; i < N_OF_INODES - 2; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        struct stat st;
        rv = ex_getattr(buffer, &st);
        g_assert(!rv);
    }

    // check that we cannot remove the populated directory
    rv = ex_rmdir("/dir");
    g_assert(rv);

    // check that we are able to delete all files
    for (int i = 0; i < N_OF_INODES - 2; i++) {

        snprintf(buffer, sizeof(buffer), "/dir/file%i", i);

        rv = ex_unlink(buffer);
        g_assert(!rv);
    }

    // check that we can remove the populated directory
    rv = ex_rmdir("/dir");
    g_assert(!rv);

    ex_deinit();
}

int main(int argc, char **argv) {
    ex_set_log_level(fatal);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/exfuse/test_creation_and_deletion",
            test_creation_and_deletion);

    return g_test_run();
}
