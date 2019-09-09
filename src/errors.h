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

inline void ex_status_report(ex_status status, enum loglevel level) {
    const char * msg = NULL;

    switch (status) {
        case DEVICE_CANNOT_BE_OPENED:
            msg = "Unable to open device";
            break;
        default:
            msg = "OK";
            break;
    }

    ex_log_format(level, "%s", msg);
}

#endif /* ERRORS_H */
