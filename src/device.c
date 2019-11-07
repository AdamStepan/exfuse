#include "device.h"

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
        error("unable to open device: %s, errno: %d", device_name,
              strerror(errno));
        return DEVICE_CANNOT_BE_OPENED;
    }

    info("device is open: fd=%d", device_fd);

    return OK;
}

ex_status ex_device_close(void) {

    if (device_fd == -1) {
        error("unable to close device because it's not opened");
        return DEVICE_IS_NOT_OPEN;
    }

    info("closing device: %i", device_fd);
    (void)close(device_fd);

    return OK;
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

    if (readed_ != (ssize_t)amount) {
        status = READ_FAILED;
    }

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
    case READ_FAILED:
        error("read: readed=%lu, wanted=%lu", *readed, amount);
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
