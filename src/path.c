#include "path.h"

struct ex_path *ex_path_make(const char *path) {

    struct ex_path *p = ex_malloc(sizeof(struct ex_path));

    // XXX: split_by_string doesn't handle paths with escaped '/'
    p->components = ex_str_split(path, "/");

    p->basename_to_free = ex_strdup(path);
    p->dirname_to_free = ex_strdup(path);

    p->basename = basename(p->basename_to_free);
    p->dirname = dirname(p->dirname_to_free);

    p->ncomponents = 0;

    // we can return count of components from split_by_string function
    while (p->components[p->ncomponents]) {
        p->ncomponents++;
    }

    return p;
}

struct ex_path *ex_path_make_dirpath(const char *pathname) {

    char *copy_of_path = ex_strdup(pathname);
    struct ex_path *path = ex_path_make(dirname(copy_of_path));

    free(copy_of_path);

    return path;
}

void ex_path_free(struct ex_path *path) {

    free(path->dirname_to_free);
    free(path->basename_to_free);

    for (size_t i = 0; i < path->ncomponents; i++) {
        free(path->components[i]);
    }

    free(path->components);
    free(path);
}

void ex_path_print(const struct ex_path *path) {

    printf("basename: %s\n", path->basename);
    printf("dirname: %s\n", path->dirname);
    printf("ncomponents: %lu\n", path->ncomponents);
    printf("components:");

    for (size_t i = 0; path->components[i]; i++) {
        printf(" %s", path->components[i]);
    }

    printf("\n");
}
