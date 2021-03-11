/**
 * @file util.h
 * This file contains various util functions.
 */
#ifndef EX_UTIL_H
#define EX_UTIL_H

#include <time.h>
#include <sys/stat.h>
#include <stdint.h>

/** This enum defines file permissions */
typedef enum {
    /** Permistion to read. */
    EX_READ = 4,
    /** Permission to write */
    EX_WRITE = 2,
    /** Permission to execute */
    EX_EXECUTE = 1,
} ex_permission;

/** Allocate the memory, call memset on it, or call fatal. */
void *ex_malloc(size_t size);

/** Reallocate the memory or log fatal error. */
void *ex_realloc(void *ptr, size_t size);

/** Duplicate the string or log fatal error */
char *ex_strdup(const char *s);

/** Count occurences of `needle` in the `haystack`. */
size_t ex_str_cnt(const char *haystack, const char *needle);

/** Split string by the delimiter. */
char **ex_str_split(const char *str, const char *delim);

/** Print the file mode. */
void ex_print_mode(mode_t m);

/** Log the file mode. */
void ex_log_mode(mode_t m);

/** Fill `dest` with current timespec. */
void ex_update_time_ns(struct timespec *dest);

/** Try to parse the uint64 from the `arg`. */
int ex_cli_parse_number(const char *option, const char *arg, uint64_t *);

/** Write human readable size to the buffer. */
void ex_readable_size(char *buf, size_t bufsize, size_t size);

/** Return human readable options used by open(2).
 *
 * This function is not reentrant.
 */
char *ex_readable_open_options(int opts);

#endif /* EX_UTIL_H */
