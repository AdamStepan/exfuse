#include "logging.h"

enum loglevel current_level = debug;


const char *level2str[] = {
    "debug", "info", "warning", "error", "fatal"
};

void ex_set_log_level(enum loglevel new_level) {
    current_level = new_level;
}

enum loglevel ex_get_log_level(void) {
    return current_level;
}
