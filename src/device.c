#include "device.h"
#include "errors.h"
#include "logging.h"
#include "util.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

const char *const EX_DEVICE = "exdev";
int device_fd = -1;

ex_status ex_device_fd(int *fd) {

    if (device_fd == -1) {
        return DEVICE_IS_NOT_OPEN;
    }

    *fd = device_fd;

    return OK;
}

ex_status ex_device_open(const char *device_name) {

    device_fd = open(device_name, O_RDWR);

    if (device_fd == -1) {
        error("unable to open device: %s, errno: %s", device_name,
              strerror(errno));
        return DEVICE_CANNOT_BE_OPENED;
    }

    info("device is open: fd=%d", device_fd);

    return OK;
}

ex_status ex_device_close(void) {

    ex_status status = OK;
    char buffer[128];

    info("closing device: %i", device_fd);

    if (device_fd == -1) {
        status = DEVICE_IS_NOT_OPEN;
        goto failure;
    }

    if (close(device_fd) == -1) {
        status = CLOSE_FAILED;
        goto failure;
    }

    return status;

failure:

    switch (status) {
        case DEVICE_IS_NOT_OPEN:
            error("unable to close device because it's not opened");
            break;
        case CLOSE_FAILED:
            (void)strerror_r(errno, buffer, sizeof(buffer));
            error("unable to close device: %s", buffer);
            break;
        default:
            warn("unhandled error: %d", status);
            break;
    }

    return status;
}

int ex_is_device_opened(void) { return device_fd != -1; }

ex_status ex_device_read(void **buffer, size_t off, size_t amount) {

    *buffer = ex_malloc(amount);
    ssize_t readed = 0;
    ex_status status = ex_device_read_to_buffer(&readed, *buffer, off, amount);

    if (status != OK) {
        free(*buffer);
        *buffer = NULL;
    }

    return status;
}

ex_status ex_device_read_to_buffer(ssize_t *readed, char *buffer, size_t off,
                                   size_t amount) {

    int fd = -1;
    ex_status status = OK;

    if ((status = ex_device_fd(&fd)) != OK) {
        status = DEVICE_IS_NOT_OPEN;
        goto failure;
    }

    off_t offset = lseek(fd, off, SEEK_SET);

    if ((off_t)off < 0) {
        status = INVALID_OFFSET;
        goto failure;
    }

    if ((size_t)offset != off) {
        status = OFFSET_SEEK_FAILED;
        goto failure;
    }

    ssize_t readed_ = read(fd, buffer, amount);

    if (readed != NULL) {
        *readed = readed_;
    }

    return OK;

failure:

    switch (status) {
    case DEVICE_IS_NOT_OPEN:
        error("device is not opened");
        break;
    case INVALID_OFFSET:
        error("lseek: underthrow (off > max(int))");
        break;
    case OFFSET_SEEK_FAILED:
        error("lseek: offset=%li, wanted=%lu", offset, off);
        break;
    default:
        error("unhandled error: %i", status);
    }

    return status;
}

ex_status ex_device_write(size_t off, const char *data, size_t amount) {

    int fd = -1;
    ex_status status = OK;

    if ((status = ex_device_fd(&fd)) != OK) {
        status = DEVICE_IS_NOT_OPEN;
        goto failure;
    }

    off_t offset = lseek(fd, off, SEEK_SET);

    if ((off_t)off < 0) {
        status = INVALID_OFFSET;
        goto failure;
    }

    if ((size_t)offset != off) {
        status = OFFSET_SEEK_FAILED;
        goto failure;
    }

    size_t written = write(fd, data, amount);

    if (written != amount) {
        status = WRITE_FAILED;
        goto failure;
    }

    return status;

failure:

    switch (status) {
    case DEVICE_IS_NOT_OPEN:
        error("device is not opened");
        break;
    case INVALID_OFFSET:
        error("lseek: underthrow (off > max(int))");
        break;
    case OFFSET_SEEK_FAILED:
        error("lseek: offset=%li, off=%lu", offset, off);
        break;
    case WRITE_FAILED:
        error("write: written=%lu, amount=%lu", written, amount);
        break;
    default:
        error("unhandled error: %i", status);
    }

    return WRITE_FAILED;
}
