/**
 * @file path.h
 *
 * This class contains an internal path implementation and the API
 */
#ifndef PATH_H
#define PATH_H

#include <sys/types.h>

/** This class defines internal implementation of the path. */
struct ex_path {
    /** Path up to (not including) final. '/' */
    char *dirname;
    /** Path from final '/' to the end. */
    char *name;
    /** Should we free dirname during deallocation? */
    char *dirname_to_free;
    /** Should we free basename during deallocation? */
    char *basename_to_free;
    /** Path components splitted by '/'.
     *
     * So if we have path = '/a/b/c', components will be {"a", "b", "c"}.
     */
    char **components;
    /** Number of the components. */
    size_t ncomponents;
};

/** Create ex_path from a string. */
struct ex_path *ex_path_make(const char *path);
/** Create ex_path from string, ignore the last path component. */
struct ex_path *ex_path_make_dirpath(const char *path);
/** Free dynamic memory used by ex_path. */
void ex_path_free(struct ex_path *path);
/** Print path to the stdout. */
void ex_path_print(const struct ex_path *path);
/** Check if path represents '/'. */
int ex_path_is_root(const struct ex_path *const path);

#endif /* PATH_H */
