#include "logging.h"
#include "mkfs.h"

int main(int argc, char **argv) {

    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    switch(ex_mkfs_parse_options(&params, argc, argv)) {
        case EX_MKFS_OPTION_PARSE_ERROR:
            return 1;
        case EX_MKFS_OPTION_HELP:
            return 0;
        default:
            ;
    }

    ex_mkfs_check_params(&params);

    return ex_mkfs(&params);
}
