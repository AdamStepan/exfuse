set(EXFUSE_LIB_SRC ../src/device.c ../src/ex.c ../src/inode.c ../src/logging.c ../src/path.c ../src/super.c ../src/util.c ../src/mkfs.c)
FILE(GLOB TEST_SOURCES *.c)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_executable(testexfuse ${TEST_SOURCES} ${EXFUSE_LIB_SRC})
target_link_libraries(testexfuse
    PRIVATE ${GLIB_LDFLAGS} m
    PRIVATE Threads::Threads)

target_include_directories(testexfuse PUBLIC ${GLIB_INCLUDE_DIRS})
add_test(testexfuse ${CMAKE_CURRENT_BINARY_DIR}/testexfuse)
