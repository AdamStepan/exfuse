set(EXFUSE_LIB_SRC
    ../src/device.c
    ../src/ex.c
    ../src/inode.c
    ../src/logging.c
    ../src/path.c
    ../src/super.c
    ../src/util.c
    ../src/mkfs.c
)

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
)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_executable(test_exfuse ${TEST_SOURCES} ${EXFUSE_LIB_SRC})
add_executable(test_stress test_stress.c ${EXFUSE_LIB_SRC})
add_executable(test_util test_util.c ${EXFUSE_LIB_SRC})

target_link_libraries(test_exfuse
    PRIVATE ${GLIB_LDFLAGS} m
    PRIVATE Threads::Threads)

target_link_libraries(test_stress
    PRIVATE ${GLIB_LDFLAGS} m
    PRIVATE Threads::Threads)

target_link_libraries(test_util
    PRIVATE ${GLIB_LDFLAGS} m
    PRIVATE Threads::Threads)

target_include_directories(test_exfuse PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(test_stress PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(test_util PUBLIC ${GLIB_INCLUDE_DIRS})

add_test(test_exfuse ${CMAKE_SOURCE_DIR}/build/test/test_exfuse)
add_test(test_stress ${CMAKE_SOURCE_DIR}/build/test/test_stress)
add_test(test_util ${CMAKE_SOURCE_DIR}/build/test/test_util)
