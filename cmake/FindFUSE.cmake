pkg_check_modules(PC_FUSE REQUIRED fuse)

find_path(FUSE_INCLUDE_DIR fuse.h
    HINTS ${PC_FUSE_INCLUDEDIR} ${PC_FUSE_INCLUDE_DIRS}
    PATH_SUFFIXES fuse libfuse)

mark_as_advanced(FUSE_INCLUDE_DIR)

find_library(FUSE_LDFLAGS
        NAMES fuse libfuse
        HINTS ${PC_FUSE_LIBDIR} ${PC_FUSE_LIBRARY_DIRS})
mark_as_advanced(FUSE_LDFLAGS)

set(FUSE_COMPILE_DEFINITIONS ${PC_FUSE_CFLAGS_OTHER})
set(FUSE_COMPILE_OPTIONS "-I${FUSE_INCLUDE_DIR}")

find_package_handle_standard_args("FUSE"
    DEFAULT_MSG
    FUSE_COMPILE_OPTIONS
    FUSE_COMPILE_DEFINITIONS
    FUSE_LDFLAGS)

mark_as_advanced(FUSE_COMPILE_DEFINITIONS FUSE_COMPILE_OPTIONS FUSE_LDFLAGS)
