#ifndef EX_UTIL_H
#define EX_UTIL_H

#include "logging.h"
#include <err.h>
#include <errno.h>
#include <linux/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void *ex_malloc(size_t size);
void *ex_realloc(void *ptr, size_t size);
char *ex_strdup(const char *s);
size_t ex_str_cnt(const char *haystack, const char *needle);
char **ex_str_split(const char *str, const char *delim);
void ex_print_permissions(const char *prefix, uint8_t mode);
void ex_print_mode(mode_t m);
void ex_update_time_ns(struct timespec *dest);

#endif /* EX_UTIL_H */
