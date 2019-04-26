pkg_check_modules(PC_FUSE REQUIRED fuse)

find_path(FUSE_INCLUDE_DIR fuse.h
		HINTS ${PC_FUSE_INCLUDEDIR} ${PC_FUSE_INCLUDE_DIRS}
		PATH_SUFFIXES fuse libfuse)

find_library(FUSE_LDFLAGS
		NAMES fuse libfuse
		HINTS ${PC_FUSE_LIBDIR} ${PC_FUSE_LIBRARY_DIRS})

set(FUSE_CFLAGS "-I${FUSE_INCLUDE_DIR} ${PC_FUSE_CFLAGS_OTHER}")

find_package_handle_standard_args("FUSE"
    DEFAULT_MSG
    FUSE_CFLAGS
    FUSE_LDFLAGS)

mark_as_advanced(FUSE_CFLAGS FUSE_LDFLAGS)
