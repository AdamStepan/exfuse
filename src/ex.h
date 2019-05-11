#ifndef EX_H
#define EX_H

#include "device.h"
#include "inode.h"
#include "super.h"
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <sys/statvfs.h>
#include <time.h>

void ex_init(const char *device);
void ex_deinit(void);

int ex_create(const char *pathname, mode_t mode);
int ex_getattr(const char *pathname, struct stat *st);
int ex_unlink(const char *pathname);
int ex_read(const char *pathname, char *buffer, size_t size, off_t offset);
int ex_write(const char *path, const char *buf, size_t size, off_t offset);
int ex_open(const char *path);
int ex_mkdir(const char *pathname, mode_t mode);
int ex_readdir(const char *pathname, struct ex_dir_entry ***inodes);
int ex_utimens(const char *pathname, const struct timespec tv[2]);
int ex_truncate(const char *path);
int ex_link(const char *src_pathname, const char *dest_pathname);
int ex_rmdir(const char *pathname);
int ex_statfs(struct statvfs *buffer);
int ex_chmod(const char *pathname, mode_t mode);
int ex_access(const char *pathname, int mode);
int ex_symlink(const char *target, const char *link);
int ex_readlink(const char *link, char *buffer, size_t bufsize);
int ex_rename(const char *from, const char *to);

#endif /* EX_H */
