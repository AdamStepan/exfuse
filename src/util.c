#include <util.h>

void *ex_malloc(size_t size) {

   void *buff = malloc(size);

   if(buff) {
      memset(buff, '\0', size);
   } else {
      err(errno, "malloc");
   }

   return buff;
}

void *ex_realloc(void *ptr, size_t size) {

    void *newptr = realloc(ptr, size);

    if(!newptr) {
        err(errno, "malloc");
    }

    return newptr;
}

char *ex_strdup(const char *s) {

    char *dup = strdup(s);

    if(!dup) {
        err(errno, "strdup");
    }

    return dup;
}

size_t ex_str_cnt(const char *haystack, const char *needle) {

    size_t occurences = 0;
    const char *tmp = haystack;

    while((tmp = strstr(tmp, needle))) {
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

    while((token = strtok_r(token ? NULL : src, delim, &lastpos))) {
        tokens[i++] = ex_strdup(token);
    }

    free(src);

    return tokens;
}

void ex_print_permissions(const char *prefix, uint8_t mode) {
    printf("%s=%c%c%c ", prefix,
        mode & 0x4 ? 'r' : '-',
        mode & 0x2 ? 'w' : '-',
        mode & 0x1 ? 'x' : '-');
}

void ex_print_mode(mode_t m) {

    if(m & S_IFDIR)
        printf("dir");

    if(m & S_ISUID)
        printf("suid ");

    if(m & S_ISGID)
        printf("sgid ");

    if(m & S_ISVTX)
        printf("sticky ");

    ex_print_permissions("other", m & 7);
    ex_print_permissions("group", (m >> 3) & 7);
    ex_print_permissions("user", (m >> 6) & 7);

    printf("\n");
}

void ex_update_time_ns(struct timespec *dest) {

    struct timespec now;

    if(clock_gettime(CLOCK_REALTIME, &now) == -1) {
        warning("clock_gettime: errno=%d (time will remain the same)", errno);
    } else {
        dest->tv_sec = now.tv_sec;
        dest->tv_nsec = now.tv_nsec;
    }
}
