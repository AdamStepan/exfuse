#include "logging.h"

enum loglevel current_level = info;

const char *level2str[] = {
    "debug", "info", "warning", "error"
};

void set_log_level(enum loglevel new_level) {
    current_level = new_level;
}

enum loglevel get_log_level(void) {
    return current_level;
}
