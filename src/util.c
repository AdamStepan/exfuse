#include "util.h"

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

void ex_print_permissions(const char *prefix, uint8_t mode) {

    char r = mode & 0x4 ? 'r' : '-';
    char w = mode & 0x2 ? 'w' : '-';
    char x = mode & 0x1 ? 'x' : '-';

    info("%s=%c%c%c ", prefix, r, w, x);
}

void ex_print_mode(mode_t m) {

    if ((m & S_IFDIR) == S_IFDIR)
        info("dir ");

    if ((m & S_IFREG) == S_IFREG)
        info("regular ");

    if ((m & S_IFLNK) == S_IFLNK)
        info("symlink ");

    if (m & S_ISUID)
        info("suid ");

    if (m & S_ISGID)
        info("sgid ");

    if (m & S_ISVTX)
        info("sticky ");

    ex_print_permissions("other", m & 7);
    ex_print_permissions("group", (m >> 3) & 7);
    ex_print_permissions("user", (m >> 6) & 7);
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
