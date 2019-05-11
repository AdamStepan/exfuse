#ifndef EX_LOGGING_H
#define EX_LOGGING_H

#include <string.h>

#define __FILENAME__                                                           \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __ex_log(level, fmt, ...)                                              \
    if (use_syslog) {                                                          \
        ex_log(level, "%s: %s: " fmt "\n", __FILENAME__, __func__,             \
               ##__VA_ARGS__);                                                 \
    } else {                                                                   \
        ex_log(level, "%s: %s: %s: " fmt "\n", level2str[level], __FILENAME__, \
               __func__, ##__VA_ARGS__);                                       \
    }

#define debug(fmt, ...) __ex_log(debug, fmt, ##__VA_ARGS__)
#define info(fmt, ...) __ex_log(info, fmt, ##__VA_ARGS__)
#define warning(fmt, ...) __ex_log(warning, fmt, ##__VA_ARGS__)
#define error(fmt, ...) __ex_log(error, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...) __ex_log(fatal, fmt, ##__VA_ARGS__)

enum loglevel { debug = 0, info, warning, error, fatal, notset };

extern const int fatal_error_exit_code;
extern const char *level2str[notset];

extern enum loglevel current_level;
extern int use_syslog;

void ex_set_log_level(enum loglevel new_level);
void ex_logging_init(const char *loglevel, int foreground);
void ex_logging_deinit(int foreground);
void ex_log(enum loglevel lvl, const char *format, ...);

enum loglevel ex_get_log_level(void);
enum loglevel ex_parse_log_level(const char *name);

#endif /* EX_LOGGING_H */
