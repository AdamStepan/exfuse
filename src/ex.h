#ifndef EX_H
#define EX_H

#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <inode.h>
#include <device.h>

void ex_init(void);
void ex_deinit(void);

int ex_create(const char *pathname, mode_t mode);
int ex_getattr(const char *pathname, struct stat *st);
int ex_unlink(const char *pathname);
int ex_read(const char *pathname, char *buffer, size_t size, off_t offset);
int ex_write(const char *path, const char *buf, size_t size, off_t offset);
int ex_open(const char *path);
int ex_mkdir(const char *pathname, mode_t mode);
int ex_readdir(const char *pathname, struct ex_inode ***inodes);
int ex_utimens(const char *pathname, const struct timespec tv[2]);

#endif /* EX_H */
