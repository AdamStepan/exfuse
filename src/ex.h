#ifndef EX_H
#define EX_H

#include "errors.h"
#include <fcntl.h>
#include <stdint.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <time.h>

struct ex_dir_entry;

ex_status ex_init(const char *device);
void ex_deinit(void);

int ex_create(const char *pathname, mode_t mode, gid_t gid, uid_t uid);
int ex_getattr(const char *pathname, struct stat *st);
int ex_unlink(const char *pathname);
int ex_read(const char *pathname, char *buffer, size_t size, off_t offset);
int ex_write(const char *path, const char *buf, size_t size, off_t offset);
int ex_open(const char *path, int mode, gid_t gid, uid_t uid);
int ex_mkdir(const char *pathname, mode_t mode, gid_t gid, uid_t uid);
int ex_readdir(const char *pathname, struct ex_dir_entry ***inodes);
int ex_utimens(const char *pathname, const struct timespec tv[2]);
int ex_truncate(const char *path, off_t size);
int ex_link(const char *src_pathname, const char *dest_pathname);
int ex_rmdir(const char *pathname);
int ex_statfs(struct statvfs *buffer);
int ex_chmod(const char *pathname, mode_t mode);
int ex_access(const char *pathname, int mode);
int ex_symlink(const char *target, const char *link);
int ex_readlink(const char *link, char *buffer, size_t bufsize);
int ex_rename(const char *from, const char *to);
int ex_chown(const char *path, uid_t uid, gid_t gid);
int ex_opendir(const char *path, mode_t mode, gid_t gid, uid_t uid);
int ex_setxattr(const char* path, const char* name, const char* value, size_t valuesize, int flags);
int ex_getxattr(const char* pathname, const char* name, void* value, size_t valuesize);

#endif /* EX_H */
