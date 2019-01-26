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

enum loglevel ex_parse_log_level(const char *name) {

    for(size_t i = 0; i < sizeof(level2str) / sizeof(level2str[0]); i++) {

        if(!strcmp(name, level2str[i])) {
            return (enum loglevel)i;
        }
    }

    return notset;
}

void ex_logging_init(const char *loglevel) {

    if (!loglevel) {
        return;
    }

    enum loglevel level = ex_parse_log_level(loglevel);

    if (level == notset) {
        warning("invalid log level: %s", loglevel);
    } else {
        ex_set_log_level(level);
    }

}



