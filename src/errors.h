#ifndef ERRORS_H
#define ERRORS_H

#include "logging.h"

typedef enum {
    DEVICE_CANNOT_BE_OPENED,
    DEVICE_IS_NOT_OPEN,
    DEVICE_SIZE_CHANGE_FAILED,
    READ_FAILED,
    WRITE_FAILED,
    INVALID_OFFSET,
    OFFSET_SEEK_FAILED,
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
