# find glib
pkg_search_module(GLIB REQUIRED glib-2.0)

FILE(GLOB TEST_SOURCES *.c)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GLIB_LDFLAGS}")

include_directories(${GLIB_INCLUDE_DIRS})

add_executable(testexfuse ${TEST_SOURCES} ${EXFUSE_SRC})
target_link_libraries(testexfuse libexfuse)
