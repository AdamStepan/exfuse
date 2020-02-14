if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "[Cc]lang")
    message(FATAL_ERROR "Code coverage is currently supported only for clang")
endif()

if(NOT TESTS)
    message(FATAL_ERROR "Cannot generate coverage report when tests are disabled")
endif()

find_program(LLVM_COV llvm-cov llvm-cov-7 llvm-cov-8 llvm-cov-5.0 HINTS /usr/bin)

if(NOT LLVM_COV)
    message(FATAL_ERROR "Cannot find llvm-cov")
endif()

find_program(LLVM_PROFDATA llvm-profdata llvm-profdata-7 llvm-profdata-8 HINTS /usr/bin)

if(NOT LLVM_PROFDATA)
    message(FATAL_ERROR "Cannot find llvm-profdata")
endif()

set(TEST_DIR ${CMAKE_BINARY_DIR}/test)

add_custom_target(coverage-profraw
    DEPENDS test_exfuse
    BYPRODUCTS exfuse.profraw
    WORKING_DIRECTORY ${TEST_DIR}
    COMMAND LLVM_PROFILE_FILE=exfuse.profraw ./test_exfuse
)

add_custom_target(coverage-profdata
    DEPENDS coverage-profraw
    BYPRODUCTS exfuse.profdata
    WORKING_DIRECTORY ${TEST_DIR}
    COMMAND ${LLVM_PROFDATA} merge -sparse exfuse.profraw -o exfuse.profdata
)

add_custom_target(coverage-report
    DEPENDS coverage-profdata
    WORKING_DIRECTORY ${TEST_DIR}
    VERBATIM
    COMMAND ${LLVM_COV} report -ignore-filename-regex "\(.*test.*\|.*glib.*\)" test_exfuse -instr-profile exfuse.profdata
)

add_custom_target(coverage-show
    DEPENDS coverage-profdata
    WORKING_DIRECTORY ${TEST_DIR}
    VERBATIM
    COMMAND ${LLVM_COV} show -ignore-filename-regex "\(.*test.*\|.*glib.*\)" test_exfuse -instr-profile exfuse.profdata
)

add_custom_target(coverage DEPENDS coverage-report)

add_compile_options(-fprofile-instr-generate -fcoverage-mapping)

set(COVERAGE_OPTIONS "-fcoverage-mapping -fprofile-instr-generate")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_OPTIONS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${COVERAGE_OPTIONS}")
