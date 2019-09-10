#ifndef EX_DEVICE_H
#define EX_DEVICE_H

#include "errors.h"
#include "logging.h"
#include "util.h"
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

extern const char *EX_DEVICE;

ex_status ex_device_fd(int *fd);
ex_status ex_device_open(const char *device_name);
ex_status ex_device_close(void);

int ex_is_device_opened(void);

ex_status ex_device_read(void **data, size_t off, size_t amount);
ex_status ex_device_read_to_buffer(ssize_t *readed, char *buffer, size_t off,
                                   size_t amount);
ex_status ex_device_write(size_t off, const char *data, size_t amount);

#endif
