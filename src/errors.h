#ifndef ERRORS_H
#define ERRORS_H

#include "logging.h"
#include <stdarg.h>

typedef enum {
    // device errors
    DEVICE_CANNOT_BE_OPENED,
    DEVICE_IS_NOT_OPEN,
    DEVICE_SIZE_CHANGE_FAILED,
    READ_FAILED,
    WRITE_FAILED,
    INVALID_OFFSET,
    OFFSET_SEEK_FAILED,
    // super block/bitmap errors
    INODE_BITMAP_IS_FULL,
    DATA_BITMAP_IS_FULL,
    BLOCK_ALLOCATION_FAILED,
    // mkfs errors
    ZEROING_OUTSIDE_OF_DEVICE_SPACE,
    DEVICE_STAT_FAILED,
    DEVICE_MMAP_FAILED,
    // inode errors
    SUPER_BLOCK_IS_NOT_LOADED,
    SUPER_BLOCK_WRITING_FAILED,
    ROOT_INODE_CANNOT_BE_CREATED,
    ROOT_CANNOT_BE_LOADED,
    INODE_BLOCK_ALLOCATION_FAILED,
    INODE_FLUSH_FAILED,
    INODE_DATA_BLOCKS_ALLOCATION_FAILED,
    INODE_CREATION_FAILED,
    INODE_LOAD_FAILED,
    INODE_IS_NOT_UNLINKABLE,
    INODE_NOT_FOUND,
    PARENT_BLOCK_CANNOT_BE_READED,
    OK
} ex_status;

inline void ex_status_report(ex_status status, enum loglevel level, ...) {

    va_list args;
    va_start(args, level);

    switch (status) {
    case DEVICE_CANNOT_BE_OPENED: {
        const char *device = device = va_arg(args, const char *);
        ex_log_format(level, "unable to open device: %s", device);
    } break;

    default:
        break;
    }

    va_end(args);
}

#endif /* ERRORS_H */
