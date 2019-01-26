#include "mkfs.h"

void help(void) {
    printf("mkfs.exfuse: \n"
        "\t--device\t\tspecify a device name\n"
        "\t--inodes\t\tspecify maximum of inodes (default: 256)\n"
        "\t--size\t\tspecify size of a device\n"
        "\t--create\t\tcreate a device if it not exist\n");
}

int main(int argc, char** argv) {

    const struct option longopts[] = {
        {"device", required_argument, 0, 'd'},
        {"inodes", required_argument, 0, 'i'},
        {"size", required_argument, 0, 's'},
        {"create", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    struct ex_mkfs_params params;
    memset(&params, '\0', sizeof(params));

    while((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
        switch(opt) {
            case 'c':
                params.create = 1;
                break;
            case 'd':
                params.device = strdup(optarg);
                break;
            case 'i':
                params.number_of_inodes = strtoull(optarg, NULL, 0);
                break;
            case 's':
                params.device_size = strtoull(optarg, NULL, 0);
                break;
            case 'h':
                help();
                return 0;
            default:
                help();
                return 1;
        }
    }

    ex_mkfs_check_params(&params);

    return ex_mkfs(&params);
}
