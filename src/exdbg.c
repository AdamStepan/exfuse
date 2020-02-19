#include "dbg.h"

int main(int argc, char **argv) {

    struct ex_dbg_options options;
    memset(&options, '\0', sizeof(options));

    if (ex_dbg_parse_options(&options, argc, argv)) {
        return 1;
    }

    return ex_dbg_run(&options);
}
