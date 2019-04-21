project(exfuse)
cmake_minimum_required(VERSION 2.8)

# check libfuse
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_FUSE REQUIRED fuse)

# get headers
find_path(FUSE_INCLUDE_DIR fuse.h
		HINTS ${PC_FUSE_INCLUDEDIR} ${PC_FUSE_INCLUDE_DIRS}
		PATH_SUFFIXES fuse libfuse)

find_library(FUSE_LIBRARIES
		NAMES fuse libfuse
		HINTS ${PC_FUSE_LIBDIR} ${PC_FUSE_LIBRARY_DIRS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${FUSE_INCLUDE_DIR} ")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FUSE_LIBRARIES}")

# check threads
find_package(Threads REQUIRED)

# add common flags
set(CFLAGS "-fPIC -Wall -I. -I../src -ggdb -std=c11 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Wextra")
set(LDFLAGS "-lm -lpthread")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LDFLAGS}")

set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
add_test(testexfuse test/testexfuse)

add_custom_target(distclean
        COMMAND find . -name "CMakeFiles" | xargs rm -rfv
        COMMAND find . -name "Makefile" | xargs rm -rfv
        COMMAND find . -name "cmake_install.cmake" | xargs rm -rfv
        COMMAND find . -name "CMakeCache.txt" | xargs rm -rfv
        COMMAND find . -name "CTestTestfile.cmake" | xargs rm -rfv
        COMMAND rm -vrf bin
        COMMAND rm -vrf lib
        COMMAND rm -vrf compile_commands.json
        COMMAND rm -vrf Testing
        COMMAND rm -vf exdev test/exdev)
