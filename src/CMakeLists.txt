set(EXFUSE_LIB_SRC device.c ex.c inode.c logging.c path.c super.c util.c mkfs.c)
set(EXFUSE_SRC wrapper.c)

add_library(libexfuse SHARED ${EXFUSE_LIB_SRC})

add_executable(exfuse ${EXFUSE_SRC})
target_link_libraries(exfuse libexfuse)

add_executable(exmkfs exmkfs.c)
target_link_libraries(exmkfs libexfuse)

add_executable(exdbg exdbg.c)
target_link_libraries(exdbg libexfuse)
