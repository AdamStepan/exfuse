#include <device.h>

int device_fd = -1;

int ex_device_fd(void) {
    return device_fd;
}

int ex_device_create(const char *name, size_t size) {

    if(access(name, F_OK) != -1) {
        return 0;
    }

    int fd = open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if(fd == -1) {
        err(errno, "%s", name);
    }

    ftruncate(fd, size);

    close(fd);

    return 1;
}

int ex_device_open(const char *device_name) {

    device_fd = open(device_name, O_RDWR);

    if(device_fd == -1) {
        err(errno, "%s", device_name);
    }

    info("device is open: fd=%d", device_fd);

    return device_fd;
}

void *ex_device_read(size_t off, size_t amount) {

    int fd = ex_device_fd();

    void *data = ex_malloc(amount);

    off_t offset = lseek(fd, off, SEEK_SET);

    if(offset != off) {
        warnx("lseek: offset=%lu, wanted=%lu", offset, off);
        goto failure;
    }

    size_t readed = read(fd, data, amount);

    if(readed != amount) {
        warnx("read: readed=%lu, wanted=%lu", readed, amount);
        goto failure;
    }

    return data;

failure:
    free(data);

    return NULL;
}

void ex_device_write(size_t off, const char *data, size_t amount) {

    int fd = ex_device_fd();

    off_t offset = lseek(fd, off, SEEK_SET);

    if(offset != off) {
        warn("lseek: offset=%lu, off=%lu", offset, off);
    }

    size_t written = write(fd, data, amount);

    if(written != amount) {
        warn("write: written=%lu, amount=%lu", written, amount);
    }
}
