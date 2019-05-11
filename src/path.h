#ifndef PATH_H
#define PATH_H

#include "util.h"
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>

struct ex_path {

    char *dirname;
    char *basename;

    char *dirname_to_free;
    char *basename_to_free;

    char **components;
    size_t ncomponents;
};

struct ex_path *ex_path_make(const char *path);
struct ex_path *ex_path_make_dirpath(const char *path);
void ex_path_free(struct ex_path *path);
void ex_path_print(const struct ex_path *path);

#endif /* PATH_H */
