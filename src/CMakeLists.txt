set(EXFUSE_LIB_SRC device.c ex.c inode.c logging.c path.c super.c util.c mkfs.c dbg.c)
set(EXFUSE_SRC wrapper.c)

find_package(Threads REQUIRED)
add_library(libexfuse STATIC ${EXFUSE_LIB_SRC})
set_target_properties(libexfuse PROPERTIES OUTPUT_NAME libexfuse
                                           PREFIX "")
target_link_libraries(libexfuse PUBLIC m PUBLIC Threads::Threads INTERFACE ${FUSE_LDFLAGS})

add_executable(exfuse ${EXFUSE_SRC})
target_link_libraries(exfuse PUBLIC libexfuse)

add_executable(exmkfs exmkfs.c)
target_link_libraries(exmkfs PUBLIC libexfuse)

add_executable(exdbg exdbg.c)
target_link_libraries(exdbg PUBLIC libexfuse)
