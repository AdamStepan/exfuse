#include <err.h>
#include <ex.h>
#include <glib.h>
#include <mkfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_inode_symlink(void) {

    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new file
    int rv = ex_create("/target", S_IRWXU);
    g_assert(!rv);

    rv = ex_symlink("/target", "/link");
    g_assert(!rv);

    char buffer[512];

    // check that symlink now points to '/target'
    rv = ex_readlink("/target", buffer, sizeof(buffer));
    g_assert(!rv);
    g_assert_cmpstr("/target", !=, buffer);

    struct stat statbuf;

    rv = ex_getattr("/link", &statbuf);
    g_assert(!rv);
    g_assert(statbuf.st_mode & S_IFLNK);

    ex_deinit();
}
