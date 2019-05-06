cmake_minimum_required(VERSION 3.7)
project(exfuse VERSION 0.0.1 LANGUAGES C)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
find_package(FUSE REQUIRED)

option(COVERAGE "Enable clang coverage flags" OFF)
option(TESTS "Enable tests" OFF)
option(DEBUG "Enable debug build" OFF)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_definitions(-D_GNU_SOURCE)
add_definitions(${FUSE_COMPILE_DEFINITIONS})

add_compile_options(-Wall -Wextra)
add_compile_options(${FUSE_COMPILE_OPTIONS})

if (COVERAGE)

    if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "[Cc]lang")
        message(FATAL_ERROR "Code coverage is currently supported only for clang")
    endif()

    add_compile_options($<$<C_COMPILER_ID:Clang>:-fprofile-instr-generate>)
    add_compile_options($<$<C_COMPILER_ID:Clang>:-fcoverage-mapping>)

    find_program(LLVM_COV llvm-cov llvm-cov-7 llvm-cov-5.0 HINTS /usr/bin)

    if(NOT LLVM_COV)
        message(FATAL_ERROR "Cannot find llvm-cov")
    endif()

    find_program(LLVM_PROFDATA llvm-profdata llvm-profdata-7 HINTS /usr/bin)

    if(NOT LLVM_PROFDATA)
        message(FATAL_ERROR "Cannot find llvm-profdata")
    endif()

    add_custom_target(coverage
        DEPENDS ${CMAKE_SOURCE_DIR}/test/testexfuse
        COMMAND LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/exfuse.profraw ${CMAKE_SOURCE_DIR}/test/testexfuse
        COMMAND ${LLVM_PROFDATA} merge -sparse ${CMAKE_BINARY_DIR}/exfuse.profraw -o ${CMAKE_BINARY_DIR}/exfuse.profdata
        COMMAND ${LLVM_COV} report -ignore-filename-regex "\\(.*test.*\\|.*glib.*\\)" ${CMAKE_SOURCE_DIR}/test/testexfuse -instr-profile ${CMAKE_BINARY_DIR}/exfuse.profdata)

endif()

if (TESTS)
    enable_testing()
endif()

if (DEBUG)
    add_compile_options($<$<C_COMPILER_ID:Clang>:-ggdb>)
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


