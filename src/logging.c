#include "logging.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

enum loglevel current_level = debug;
int use_syslog = 0;

const char *level2str[] = {"debug", "info", "warning", "error", "fatal"};
const int fatal_error_exit_code = 42;

void ex_set_log_level(enum loglevel new_level) { current_level = new_level; }

enum loglevel ex_get_log_level(void) { return current_level; }

enum loglevel ex_parse_log_level(const char *name) {

    for (size_t i = 0; i < sizeof(level2str) / sizeof(level2str[0]); i++) {

        if (!strcmp(name, level2str[i])) {
            return (enum loglevel)i;
        }
    }

    return notset;
}

void ex_logging_init(const char *loglevel, int foreground) {

    if (!foreground) {
        openlog("exfuse", LOG_PID, LOG_DAEMON);
        use_syslog = 1;
    }

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

void ex_log(enum loglevel lvl, const char *format, ...) {

    if (current_level > lvl) {
        return;
    }

    va_list args;
    va_start(args, format);

    if (use_syslog) {

        int syslog_level = LOG_NOTICE;

        switch (lvl) {
        case debug:
            syslog_level = LOG_DEBUG;
            break;
        case info:
            syslog_level = LOG_INFO;
            break;
        case warning:
            syslog_level = LOG_WARNING;
            break;
        case error:
            syslog_level = LOG_ERR;
            break;
        case fatal:
            syslog_level = LOG_ALERT;
            break;
        case notset:
            break;
        }

        vsyslog(syslog_level, format, args);

    } else {
        vprintf(format, args);
    }

    va_end(args);

    if (lvl >= fatal) {
        closelog();
        exit(fatal_error_exit_code);
    }
}
