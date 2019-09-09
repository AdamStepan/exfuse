#include "device.h"


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
        error("unable to open device: %s, errno: %d", device_name, errno);
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

int ex_is_device_opened(void) {
    return device_fd != -1;
}

void *ex_device_read(size_t off, size_t amount) {

    void *buffer = ex_malloc(amount);
    size_t readed = ex_device_read_to_buffer(buffer, off, amount);

    if (readed != amount) {
        warning("read: readed=%lu, wanted=%lu", readed, amount);
        goto failure;
    }

    return buffer;

failure:
    free(buffer);

    return NULL;
}

ssize_t ex_device_read_to_buffer(char *buffer, size_t off, size_t amount) {

    int fd = -1;
    ex_status status = OK;

    if ((status = ex_device_fd(&fd)) != OK) {
        warning("device is not opened");
        goto failure;
    }

    off_t offset = lseek(fd, off, SEEK_SET);

    if ((off_t)off < 0) {
        warning("lseek: underthrow (off > max(int))");
        goto failure;
    }

    if ((size_t)offset != off) {
        warning("lseek: offset=%lu, wanted=%lu", offset, off);
        goto failure;
    }

    return read(fd, buffer, amount);

failure:
    return -1;
}

void ex_device_write(size_t off, const char *data, size_t amount) {

    int fd = -1;
    ex_status status = OK;

    if ((status = ex_device_fd(&fd)) != OK) {
        warning("device is not opened");
        goto failure;
    }

    off_t offset = lseek(fd, off, SEEK_SET);

    if ((off_t)off < 0) {
        warning("lseek: underthrow (off > max(int))");
        goto failure;
    }

    if ((size_t)offset != off) {
        warning("lseek: offset=%lu, off=%lu", offset, off);
        goto failure;
    }

    size_t written = write(fd, data, amount);

    if (written != amount) {
        warning("write: written=%lu, amount=%lu", written, amount);
    }
failure:;
}
