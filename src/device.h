#ifndef EX_DEVICE_H
#define EX_DEVICE_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <logging.h>
#include <util.h>

#define EX_DEVICE "exdev"

extern int device_fd;

int ex_device_fd(void);
int ex_device_create(const char *name, size_t size);
int ex_device_open(const char *device_name);

void *ex_device_read(size_t off, size_t amount);
void ex_device_write(size_t off, const char *data, size_t amount);

#endif
