#include "device.h"

int device_fd = -1;

int ex_device_fd(void) { return device_fd; }

int ex_device_create(const char *name, size_t size) {

    if (access(name, F_OK) != -1) {
        return 0;
    }

    int fd = open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        fatal("unable to open device: %s, errno: %d", name, errno);
    }

    ftruncate(fd, size);

    close(fd);

    return 1;
}

int ex_device_open(const char *device_name) {

    device_fd = open(device_name, O_RDWR);

    if (device_fd == -1) {
        fatal("unable to open device: %s, errno: %d", device_name, errno);
    }

    info("device is open: fd=%d", device_fd);

    return device_fd;
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

    int fd = ex_device_fd();

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

    int fd = ex_device_fd();

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
