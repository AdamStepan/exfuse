#include "util.h"
#include <limits.h>
#include <fcntl.h>

void *ex_malloc(size_t size) {

    void *buff = malloc(size);

    if (buff) {
        memset(buff, '\0', size);
    } else {
        fatal("malloc failed: errno: %d", errno);
    }

    return buff;
}

void *ex_realloc(void *ptr, size_t size) {

    void *newptr = realloc(ptr, size);

    if (!newptr) {
        fatal("realloc failed: errno: %d", errno);
    }

    return newptr;
}

char *ex_strdup(const char *s) {

    char *dup = strdup(s);

    if (!dup) {
        fatal("strdup failed: errno: %d", errno);
    }

    return dup;
}

size_t ex_str_cnt(const char *haystack, const char *needle) {

    size_t occurences = 0;
    const char *tmp = haystack;

    while ((tmp = strstr(tmp, needle))) {
        occurences++;
        tmp++;
    }

    return occurences;
}

char **ex_str_split(const char *str, const char *delim) {

    size_t occurences = ex_str_cnt(str, delim);
    char **tokens = ex_malloc(sizeof(char *) * (occurences + 2));

    tokens[occurences] = NULL;

    char *token = NULL, *lastpos = NULL, *src = ex_strdup(str);
    size_t i = 0;

    while ((token = strtok_r(token ? NULL : src, delim, &lastpos))) {
        tokens[i++] = ex_strdup(token);
    }

    free(src);

    return tokens;
}

size_t ex_format_permissions(char *buffer, const char *prefix, uint8_t mode) {

    char r = mode & 0x4 ? 'r' : '-';
    char w = mode & 0x2 ? 'w' : '-';
    char x = mode & 0x1 ? 'x' : '-';

    return (size_t)sprintf(buffer, "%s=%c%c%c", prefix, r, w, x);
}

size_t ex_format_mode(char *buffer, mode_t m) {

    char *oldbuf = buffer;

    if ((m & S_IFDIR) == S_IFDIR)
        buffer += sprintf(buffer, "dir ");

    if ((m & S_IFREG) == S_IFREG)
        buffer += sprintf(buffer, "regular ");

    if ((m & S_IFLNK) == S_IFLNK)
        buffer += sprintf(buffer, "symlink ");

    if (m & S_ISUID)
        buffer += sprintf(buffer, "suid ");

    if (m & S_ISGID)
        buffer += sprintf(buffer, "sgid ");

    if (m & S_ISVTX)
        buffer += sprintf(buffer, "sticky ");

    buffer += ex_format_permissions(buffer, "other", m & 7);
    buffer += ex_format_permissions(buffer, " group", (m >> 3) & 7);
    buffer += ex_format_permissions(buffer, " user", (m >> 6) & 7);

    return buffer - oldbuf;
}

void ex_print_mode(mode_t m) {
    char buffer[256];

    ex_format_mode(buffer, m);

    printf("%s", buffer);
}

void ex_log_mode(mode_t m) {
    char buffer[256];

    ex_format_mode(buffer, m);

    info("%s", buffer);
}

void ex_update_time_ns(struct timespec *dest) {

    struct timespec now;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
        warning("clock_gettime: errno=%d (time will remain the same)", errno);
    } else {
        dest->tv_sec = now.tv_sec;
        dest->tv_nsec = now.tv_nsec;
    }
}

int ex_cli_parse_number(const char *option, const char *arg, uint64_t *value) {

    *value = strtoull(arg, NULL, 0);

    if (*value == ULLONG_MAX) {
        error("argument `%s' supplied to option: `--%s' is not \
unsigned integer", arg, option);
        return 0;
    }

    return 1;
}

void ex_readable_size(char *buf, size_t bufsize, size_t size) {

    static const char *units[] = {"B",   "kiB", "MiB", "GiB", "TiB",
                                  "PiB", "EiB", "ZiB", "YiB"};

    int i = 0;
    double size0 = size;

    while (size0 >= 1024) {
        size0 /= 1024;
        i++;
    }

    snprintf(buf, bufsize, "%.*f%s", i, size0, units[i]);
}

char *ex_readable_open_options(int opts) {

    static char buffer[512];
    char *buf = buffer;

    if (opts & O_RDONLY)
        buf += sprintf(buf, "O_RDONLY ");

    if (opts & O_WRONLY)
        buf += sprintf(buf, "O_WRONLY ");

    if (opts & O_RDWR)
        buf += sprintf(buf, "O_RDWR ");

    if (opts & O_CREAT)
        buf += sprintf(buf, "O_CREAT ");

    if (opts & O_EXCL)
        buf += sprintf(buf, "O_EXCL ");

    if (opts & O_NOCTTY)
        buf += sprintf(buf, "O_NOCTTY ");

    if (opts & O_TRUNC)
        buf += sprintf(buf, "O_TRUNC ");

    if (opts & O_APPEND)
        buf += sprintf(buf, "O_APPEND ");

    if (opts & O_NONBLOCK)
        buf += sprintf(buf, "O_NONBLOCK ");

    if (opts & O_DSYNC)
        buf += sprintf(buf, "O_DSYNC ");

    if (opts & FASYNC)
        buf += sprintf(buf, "FASYNC ");

#ifdef O_DIRECT
    if (opts & O_DIRECT)
        buf += sprintf(buf, "O_DIRECT ");
#endif

#ifdef O_LARGEFILE
    if (opts & O_LARGEFILE)
        buf += sprintf(buf, "O_LARGEFILE ");
#endif

    if (opts & O_DIRECTORY)
        buf += sprintf(buf, "O_DIRECTORY ");

    if (opts & O_NOFOLLOW)
        buf += sprintf(buf, "O_NOFOLLOW ");

#ifdef O_NOATIME
    if (opts & O_NOATIME)
        buf += sprintf(buf, "O_NOATIME ");
#endif

    if (opts & O_CLOEXEC)
        buf += sprintf(buf, "O_CLOEXEC ");

    if (opts & O_SYNC)
        buf += sprintf(buf, "O_SYNC ");

#ifdef O_PATH
    if (opts & O_PATH)
        buf += sprintf(buf, "O_PATH ");
#endif

#ifdef O_TMPFILE
    if (opts & O_TMPFILE)
        buf += sprintf(buf, "O_TMPFILE");
#endif

    return buffer;
}

