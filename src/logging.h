#ifndef EX_LOGGING_H
#define EX_LOGGING_H

#include <string.h>

#define __FILENAME__                                                           \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define ex_log_format(level, fmt, ...)                                         \
    do {                                                                       \
        if (use_syslog) {                                                      \
            ex_log(level, "%s: %s: " fmt "\n", __FILENAME__, __func__,         \
                   ##__VA_ARGS__);                                             \
        } else {                                                               \
            ex_log(level, "%s: %s: %s: " fmt "\n", level2str[level],           \
                   __FILENAME__, __func__, ##__VA_ARGS__);                     \
        }                                                                      \
    } while (0)

#define debug(fmt, ...) ex_log_format(debug, fmt, ##__VA_ARGS__)
#define info(fmt, ...) ex_log_format(info, fmt, ##__VA_ARGS__)
#define warning(fmt, ...) ex_log_format(warning, fmt, ##__VA_ARGS__)
#define error(fmt, ...) ex_log_format(error, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...) ex_log_format(fatal, fmt, ##__VA_ARGS__)

enum loglevel { debug = 0, info, warning, error, fatal, notset };

extern const int fatal_error_exit_code;
extern const char *level2str[notset];

extern enum loglevel current_level;
extern int use_syslog;

void ex_set_log_level(enum loglevel new_level);
int ex_logging_init(const char *loglevel, int foreground);
void ex_logging_deinit(int foreground);
void ex_log(enum loglevel lvl, const char *format, ...) __attribute__((format(printf, 2, 3)));

enum loglevel ex_get_log_level(void);
enum loglevel ex_parse_log_level(const char *name);

#endif /* EX_LOGGING_H */
