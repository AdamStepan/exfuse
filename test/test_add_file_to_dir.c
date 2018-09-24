#include <linux/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ex.h>

int main(int argc, char **argv) {
    // create new device
    unlink(EX_DEVICE);
    ex_init();

    // create new directory
    int rv = ex_mkdir("/dir", S_IRWXU | S_IFDIR);

    if(rv) {
        printf("create /dir");
        goto end;
    }

    // create file
    rv = ex_create("/dir/file", S_IRWXU);

    if(rv) {
        printf("create file");
        goto end;
    }

    // check that we can getattribute for file
    struct stat st;
    rv = ex_getattr("/dir/file", &st);

    if(rv) {
        printf("getattr");
        goto end;
    }


    // check that readdir return our file
    struct ex_inode **inodes;

    rv = ex_readdir("/dir/", &inodes);

    if(rv) {
        printf("readdir");
        goto end;
    }

    for(size_t i = 0; inodes[i]; i++) {
        if(!strcmp(inodes[i]->name, "file")) {
            goto end;
        }
    }

    printf("readdir without 'file'");
    rv = 1;

end:
    ex_deinit();

    return rv;
}
