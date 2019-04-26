set(EXFUSE_LIB_SRC ../src/device.c ../src/ex.c ../src/inode.c ../src/logging.c ../src/path.c ../src/super.c ../src/util.c ../src/mkfs.c)

FILE(GLOB TEST_SOURCES *.c)

# find glib
pkg_search_module(GLIB REQUIRED glib-2.0)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GLIB_LDFLAGS}")

include_directories(${GLIB_INCLUDE_DIRS})

add_executable(testexfuse ${TEST_SOURCES} ${EXFUSE_LIB_SRC})
