#include "../src/logging.h"
#include <glib.h>

void test_write_max_size(void);
void test_truncate_file(void);
void test_unlink_file(void);
void test_repopulation_of_device(void);
void test_statfs(void);
void test_stat_time_update(void);
void test_populate_and_remove_dir(void);
void test_remove_dir(void);
void test_mkfs_device_size(void);
void test_add_file_to_dir(void);
void test_inode_symlink(void);
void test_name_too_long(void);
void test_can_read_superblock(void);
void test_file_block_deallocation(void);
void test_create_file(void);
void test_create_dir(void);
void test_can_create_maximum_inodes(void);
void test_inode_link(void);
void test_chmod(void);
void test_chown(void);
void test_inode_find(void);
void test_path_make(void);
void test_path_is_root(void);
void test_ex_mkfs_parse_inodes_arg(void);
void test_ex_mkfs_parse_log_level(void);
void test_ex_mkfs_parse_size_arg(void);
void test_unknown_option(void);
void test_has_perm_other(void);
void test_has_perm_user_and_group(void);

int main(int argc, char **argv) {
    ex_set_log_level(fatal);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/exfuse/test_write_max_size", test_write_max_size);
    g_test_add_func("/exfuse/test_truncate_file", test_truncate_file);
    g_test_add_func("/exfuse/test_unlink_file", test_unlink_file);
    g_test_add_func("/exfuse/test_repopulation_of_device",
                    test_repopulation_of_device);
    g_test_add_func("/exfuse/test_statfs", test_statfs);
    g_test_add_func("/exfuse/test_stat_time_update", test_stat_time_update);
    g_test_add_func("/exfuse/test_populate_and_remove_dir",
                    test_populate_and_remove_dir);
    g_test_add_func("/exfuse/test_remove_dir", test_remove_dir);
    g_test_add_func("/exfuse/test_mkfs_device_size", test_mkfs_device_size);
    g_test_add_func("/exfuse/test_add_file_to_dir", test_add_file_to_dir);
    g_test_add_func("/exfuse/test_inode_symlink", test_inode_symlink);
    g_test_add_func("/exfuse/test_name_too_long", test_name_too_long);
    g_test_add_func("/exfuse/test_can_read_superblock",
                    test_can_read_superblock);
    g_test_add_func("/exfuse/test_file_block_deallocation",
                    test_file_block_deallocation);
    g_test_add_func("/exfuse/test_create_file", test_create_file);
    g_test_add_func("/exfuse/test_create_dir", test_create_dir);
    g_test_add_func("/exfuse/test_can_create_maximum_inodes",
                    test_can_create_maximum_inodes);
    g_test_add_func("/exfuse/test_inode_link", test_inode_link);
    g_test_add_func("/exfuse/test_chmod", test_chmod);
    g_test_add_func("/exfuse/test_chown", test_chown);
    g_test_add_func("/exfuse/test_inode_find", test_inode_find);

    g_test_add_func("/path/test_path_make", test_path_make);
    g_test_add_func("/path/test_path_is_root", test_path_is_root);

    g_test_add_func("/exmkfs/test_ex_mkfs_parse_inodes_arg",
                    test_ex_mkfs_parse_inodes_arg);
    g_test_add_func("/exmkfs/test_ex_mkfs_parse_log_level",
                    test_ex_mkfs_parse_log_level);
    g_test_add_func("/exmkfs/test_ex_mkfs_parse_size_arg",
                    test_ex_mkfs_parse_size_arg);
    g_test_add_func("/exmkfs/test_ex_mkfs_parse_unknown_option",
                    test_unknown_option);

    g_test_add_func("/util/test_has_perm_other", test_has_perm_other);
    g_test_add_func("/util/test_has_perm_user_and_group",
                    test_has_perm_user_and_group);

    return g_test_run();
}
