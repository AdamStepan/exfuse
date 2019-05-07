#include <ex.h>
#include <glib.h>
#include <linux/stat.h>
#include <mkfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_add_file_to_dir(void) {
    // create new device
    unlink(EX_DEVICE);
    ex_mkfs_test_init();

    // create new directory
    int rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR);
    g_assert(!rv);

    // create file
    rv = ex_create("/dir/file", S_IRWXU);
    g_assert(!rv);

    // check that we can getattribute for file
    struct stat st;
    rv = ex_getattr("/dir/file", &st);
    g_assert(!rv);

    // check that readdir return our file
    struct ex_dir_entry **entries;

    rv = ex_readdir("/dir/", &entries);
    g_assert(!rv);

    for (size_t i = 0; entries[i]; i++) {
        if (!strcmp(entries[i]->name, "file")) {
            goto end;
        }
    }

    g_assert(0);

end:;
}
