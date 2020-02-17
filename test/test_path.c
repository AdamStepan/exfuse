#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_path_make(void) {
    struct ex_path *path = ex_path_make("/a/b/c/d/");

    g_assert(path->ncomponents == 4);
    g_assert(strncmp(path->dirname, "/a/b/c", 6) == 0);
    g_assert(strncmp(path->basename, "d", 1) == 0);

    ex_path_free(path);

    path = ex_path_make("/a");

    g_assert(path->ncomponents == 1);
    g_assert(strncmp(path->dirname, "/", 1) == 0);
    g_assert(strncmp(path->basename, "a", 1) == 0);

    ex_path_free(path);
}

void test_path_is_root(void) {

    struct ex_path *path = ex_path_make("/a/b/c/d/");

    g_assert(ex_path_is_root(path) == 0);
    ex_path_free(path);

    path = ex_path_make("/a");
    g_assert(ex_path_is_root(path) == 0);
    ex_path_free(path);

    path = ex_path_make("/");
    g_assert(ex_path_is_root(path) == 1);
    ex_path_free(path);
}
