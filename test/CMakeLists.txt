set(TEST_SOURCES
    test_add_file_to_dir.c
    test_can_create_maximum_inodes.c
    test_can_read_superblock.c
    test_create_dir.c
    test_create_file.c
    test_file_block_deallocation.c
    test_inode_link.c
    test_inode_symlink.c
    test_main.c
    test_mkfs_device_size.c
    test_name_too_long.c
    test_populate_and_remove_dir.c
    test_remove_dir.c
    test_repopulation_of_device.c
    test_statfs.c
    test_stat_time_update.c
    test_truncate_file.c
    test_unlink_file.c
    test_write_max_size.c
    test_chmod.c
    test_chown.c
    test_find.c
    test_util.c
    test_path.c
    test_mkfs.c
)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

function (ex_fuse_add_test TESTNAME TESTSOURCES)

    add_executable(${TESTNAME} ${TESTSOURCES})


    target_link_libraries(${TESTNAME}
        PRIVATE libexfuse
        PRIVATE ${GLIB_LDFLAGS} m
        PRIVATE Threads::Threads)

    target_include_directories(${TESTNAME} PUBLIC ${GLIB_INCLUDE_DIRS})

    add_test(${TESTNAME} ${CMAKE_SOURCE_DIR}/build/test/${TESTNAME})

endfunction()


ex_fuse_add_test(test_exfuse "${TEST_SOURCES}")
ex_fuse_add_test(test_stress test_stress.c)
