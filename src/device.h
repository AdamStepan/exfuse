#ifndef EX_DEVICE_H
#define EX_DEVICE_H

#include "logging.h"
#include "util.h"
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define EX_DEVICE "exdev"

extern int device_fd;

int ex_device_fd(void);
int ex_device_create(const char *name, size_t size);
int ex_device_open(const char *device_name);

void *ex_device_read(size_t off, size_t amount);
ssize_t ex_device_read_to_buffer(char *buffer, size_t off, size_t amount);
void ex_device_write(size_t off, const char *data, size_t amount);

#endif
