project(exfuse)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(FUSE REQUIRED)

option(COVERAGE "Enable clang coverage flags" OFF)
option(TESTS "Enable tests" OFF)
option(DEBUG "Enable debug build" OFF)

set(CFLAGS "-fPIC -Wall -I. -I../src -std=c11 -D_GNU_SOURCE -Wextra")
set(LDFLAGS "-lm ${CMAKE_THREAD_LIBS_INIT}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUSE_CFLAGS} ${CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FUSE_LDFLAGS} ${LDFLAGS}")

if (COVERAGE)
    # TODO: check that we are using clang
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

if (TESTS)
    enable_testing()
endif()

if (DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")
endif()

add_subdirectory(src)

if (TESTS)
    add_subdirectory(test)
    add_test(testexfuse test/testexfuse)
endif()

add_custom_target(distclean
        COMMAND find . -name "CMakeFiles" | xargs rm -rfv
        COMMAND find . -name "Makefile" | xargs rm -rfv
        COMMAND find . -name "cmake_install.cmake" | xargs rm -rfv
        COMMAND find . -name "CMakeCache.txt" | xargs rm -rfv
        COMMAND find . -name "CTestTestfile.cmake" | xargs rm -rfv
        COMMAND find . -name "*.profdata" | xargs rm -rfv
        COMMAND find . -name "*.profraw" | xargs rm -rfv
        COMMAND rm -vrf bin
        COMMAND rm -vrf lib
        COMMAND rm -vrf compile_commands.json
        COMMAND rm -vrf Testing
        COMMAND rm -vf exdev test/exdev)

add_custom_target(coverage
    DEPENDS ./test/testexfuse
    COMMAND LLVM_PROFILE_FILE=exfuse.profraw ./test/testexfuse
    COMMAND llvm-profdata-7 merge -sparse exfuse.profraw -o exfuse.profdata
    COMMAND llvm-cov-7 report -ignore-filename-regex "\\(.*test.*\\|.*glib.*\\)" ./test/testexfuse -instr-profile exfuse.profdata)
