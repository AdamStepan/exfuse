#ifndef EX_LOGGING_H
#define EX_LOGGING_H

#include <stdio.h>

#define ex_log(level, fmt, ...) printf(level ": %s: %s: " fmt "\n", \
        __FILE__, __func__, ##__VA_ARGS__)

#define debug(fmt, ...) ex_log("debug", fmt, ##__VA_ARGS__)
#define info(fmt, ...) ex_log("info", fmt, ##__VA_ARGS__)
#define warning(fmt, ...) ex_log("warning", fmt, ##__VA_ARGS__)
#define error(fmt, ...) ex_log("error", fmt, ##__VA_ARGS__)


#endif /* EX_LOGGING_H */
