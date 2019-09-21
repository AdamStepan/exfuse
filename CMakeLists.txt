cmake_minimum_required(VERSION 3.7)
project(exfuse VERSION 0.0.1 LANGUAGES C)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
find_package(FUSE REQUIRED)

option(COVERAGE "Enable clang coverage flags" OFF)
option(TESTS "Enable tests" OFF)
option(DEBUG "Enable debug build" OFF)
option(SANITIZER "Enable compiler sanitizer" OFF)

add_definitions(${FUSE_COMPILE_DEFINITIONS})

add_compile_options(-Wall -Wextra)
add_compile_options(${FUSE_COMPILE_OPTIONS})

if (COVERAGE)
    include(Coverage)
endif()

if (TESTS)
    enable_testing()
endif()

if (DEBUG)
    add_compile_options($<$<C_COMPILER_ID:Clang>:-ggdb>)
endif()

if (SANITIZER)
    include(Sanitizer)
endif()

add_subdirectory(src)

if (TESTS)
    add_subdirectory(test)
endif()

add_custom_target(distclean
        COMMAND find . -name "CMakeFiles" | xargs rm -rfv
        COMMAND find . -name "Makefile" | xargs rm -rfv
        COMMAND find . -name "cmake_install.cmake" | xargs rm -rfv
        COMMAND find . -name "CMakeCache.txt" | xargs rm -rfv
        COMMAND find . -name "CTestTestfile.cmake" | xargs rm -rfv
        COMMAND find . -name "*.profdata" | xargs rm -rfv
        COMMAND find . -name "*.profraw" | xargs rm -rfv
        COMMAND find . -name "exdev" | xargs rm -rfv
        COMMAND rm -vrf bin build lib
        COMMAND rm -vrf compile_commands.json
        COMMAND rm -vrf Testing
        VERBATIM
)

add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR} -iname '*.h' -o -iname '*.c' | xargs clang-format -i
)
