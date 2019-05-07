#ifndef EX_LOGGING_H
#define EX_LOGGING_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ex_log(level, fmt, ...)                                                \
    if (level >= current_level) {                                              \
        printf("%s: %s: %s: " fmt "\n", level2str[level], __FILE__, __func__,  \
               ##__VA_ARGS__);                                                 \
    }

#define debug(fmt, ...) ex_log(debug, fmt, ##__VA_ARGS__)
#define info(fmt, ...) ex_log(info, fmt, ##__VA_ARGS__)
#define warning(fmt, ...) ex_log(warning, fmt, ##__VA_ARGS__)
#define error(fmt, ...) ex_log(error, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...)                                                        \
    do {                                                                       \
        ex_log(fatal, fmt, ##__VA_ARGS__);                                     \
        exit(1);                                                               \
    } while (0);

enum loglevel { debug = 0, info, warning, error, fatal, notset };

extern const char *level2str[notset];
extern enum loglevel current_level;

void ex_set_log_level(enum loglevel new_level);
void ex_logging_init(const char *loglevel);
enum loglevel ex_get_log_level(void);
enum loglevel ex_parse_log_level(const char *name);

#endif /* EX_LOGGING_H */
