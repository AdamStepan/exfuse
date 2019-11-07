#include "../src/mkfs.h"
#include <glib.h>

void test_ex_mkfs_parse_inodes_arg() {

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    char *invalid_inodes_args[] = {"progname", "--inodes", "abcde", NULL};

    int rv = ex_mkfs_parse_options(&params, 3, invalid_inodes_args);
    assert(rv == EX_MKFS_OPTION_PARSE_ERROR);

    char *inode_args_hex[] = {"progname", "--inodes", "0x1000", NULL};

    rv = ex_mkfs_parse_options(&params, 3, inode_args_hex);
    assert(rv == EX_MKFS_OPTION_OK);
    assert(params.number_of_inodes == 0x1000);
}

void test_ex_mkfs_parse_size_arg() {

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    char *invalid_size_args[] = {"progname", "--size", "-0x10000000", NULL};

    int rv = ex_mkfs_parse_options(&params, 3, invalid_size_args);
    assert(rv == EX_MKFS_OPTION_PARSE_ERROR);

    char *invalid_size_args1[] = {"progname", "--size",
                                  "0x100000000000000000000000", NULL};

    rv = ex_mkfs_parse_options(&params, 3, invalid_size_args1);
    assert(rv == EX_MKFS_OPTION_PARSE_ERROR);

    char *valid_size_args[] = {"progname", "--size", "0x10000000000", NULL};

    rv = ex_mkfs_parse_options(&params, 3, valid_size_args);
    assert(rv == EX_MKFS_OPTION_OK);
}

void test_ex_mkfs_parse_log_level() {

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    char *invalid_log_level[] = {"progname", "--log-level", "abcde"};

    int rv = ex_mkfs_parse_options(&params, 3, invalid_log_level);
    assert(rv == EX_MKFS_OPTION_PARSE_ERROR);

    char *valid_log_level[] = {"progrname", "--log-level", "error"};

    rv = ex_mkfs_parse_options(&params, 3, valid_log_level);
    assert(rv == EX_MKFS_OPTION_OK);

    // restore log level
    ex_set_log_level(fatal);
}

void test_unknown_option() {

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    char *invalid_options[] = {"progname", "--whatever", "--abcd"};

    int rv = ex_mkfs_parse_options(&params, 3, invalid_options);
    assert(rv == EX_MKFS_OPTION_UNKNOWN);
}

int main(int argc, char **argv) {

    ex_set_log_level(fatal);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/exmkfs/test_ex_mkfs_parse_inodes_arg",
                    test_ex_mkfs_parse_inodes_arg);
    g_test_add_func("/exmkfs/test_ex_mkfs_parse_log_level",
                    test_ex_mkfs_parse_log_level);
    g_test_add_func("/exmkfs/test_ex_mkfs_parse_size_arg",
                    test_ex_mkfs_parse_size_arg);
    g_test_add_func("/exmkfs/test_ex_mkfs_parse_unknown_option",
                    test_unknown_option);

    g_test_run();
}
