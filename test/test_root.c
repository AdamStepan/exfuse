#include "../src/ex.h"
#include "../src/mkfs.h"
#include <err.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void test_root_api_without_super_block(void) {

    ex_status status = ex_root_create();
    g_assert_cmpint(status, ==, SUPER_BLOCK_IS_NOT_LOADED);

    status = ex_root_load(NULL);
    g_assert_cmpint(status, ==, SUPER_BLOCK_IS_NOT_LOADED);
}

