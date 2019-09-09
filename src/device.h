#ifndef EX_DEVICE_H
#define EX_DEVICE_H

#include "logging.h"
#include "util.h"
#include "errors.h"
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define EX_DEVICE "exdev"

ex_status ex_device_fd(int *fd);
ex_status ex_device_open(const char *device_name);
ex_status ex_device_close(void);

int ex_is_device_opened(void);

void *ex_device_read(size_t off, size_t amount);
ssize_t ex_device_read_to_buffer(char *buffer, size_t off, size_t amount);
void ex_device_write(size_t off, const char *data, size_t amount);

#endif
