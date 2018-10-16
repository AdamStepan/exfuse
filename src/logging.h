#ifndef EX_LOGGING_H
#define EX_LOGGING_H

#include <stdio.h>
#include <stdint.h>

#define ex_log(level, fmt, ...) \
    if(level >= current_level) { \
        printf("%s: %s: %s: " fmt "\n", \
            level2str[level], __FILE__, __func__, ##__VA_ARGS__); \
        \
    }

#define debug(fmt, ...) ex_log(debug, fmt, ##__VA_ARGS__)
#define info(fmt, ...) ex_log(info, fmt, ##__VA_ARGS__)
#define warning(fmt, ...) ex_log(warning, fmt, ##__VA_ARGS__)
#define error(fmt, ...) ex_log(error, fmt, ##__VA_ARGS__)

enum loglevel {
    debug = 0,
    info,
    warning,
    error
};

extern const char *level2str[];
extern enum loglevel current_level;

void set_log_level(enum loglevel new_level);
enum loglevel get_log_level(void);

#endif /* EX_LOGGING_H */
