#ifndef PATH_H
#define PATH_H

#include <dirent.h>
#include <errno.h>
#include <err.h>
#include <libgen.h>
#include <util.h>
#include <stdio.h>

struct ex_path {

    char *dirname;
    char *basename;

    char *dirname_to_free;
    char *basename_to_free;

    char **components;
    size_t ncomponents;
};

struct ex_path *ex_make_path(const char *path);
struct ex_path *ex_make_dir_path(const char *path);
void ex_free_path(struct ex_path *path);
void ex_print_path(const struct ex_path *path);

#endif /* PATH_H */
