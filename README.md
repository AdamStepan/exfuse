# Exfuse [![Build Status](https://travis-ci.org/AdamStepan/exfuse.svg?branch=master)](https://travis-ci.org/AdamStepan/exfuse)

Exfuse is a simple Unix filesystem that I created for educational purposes. Currently, its use is limited to the fuse, but is written so that it can be used as a library without it. I describe it as simple because
* An inode uses only the direct blocks
* All blocks are allocated when creating a folder/file
* Files are stored in a folder as a list
* Uses a global lock
* No duplication of key structures (e.g. superblock, root, bitmaps)

## Limitations

Because the filesystem uses only the direct addressing there is a limitation of file size and
number of entries in a directory. File size is limited to `EX_DIRECT_BLOCKS * EX_BLOCK_SIZE`, which is currently ` 256 * 4096 = 1MiB` and the maximum number of elements in a directory is  limited to `EX_DIRECT_BLOCKS * EX_BLOCK_SIZE / sizeof (struct ex_dir_entry)` which is currently `256 * 4096 / 64 = 16384 ` entries.

## List of implemented functions
    access
    chmod
    chown
    create
    getattr (e.g. stat)
    link
    mkdir
    open
    read
    readdir
    readlink
    rename
    rmdir
    statfs
    symlink
    truncate
    unlink
    utimens
    write

## Compilation

```sh
mkdir --parent build
cd build
cmake .. -DTESTS=ON
make && make test
```

## Components
### exmkfs
It is used to store filesystem structures on a "device". You can specify the maximum number of inodes during the initialization of filesystem, the default value is 256. The minimum device size for the given number of inodes is determined by the following function:
```c

size_t ex_mkfs_get_size_for_inodes(size_t ninodes) {
    // space for inodes
    size_t required = ninodes * EX_BLOCK_SIZE;
    // space for inodes bitmap
    required += round_to_block(ninodes / 8);
    // space for data of inodes
    required += ninodes * EX_DIRECT_BLOCKS * EX_BLOCK_SIZE;
    // space for data bitmap
    required += round_to_block(ninodes * EX_DIRECT_BLOCKS / 8);
    // space for super block
    required += round_to_block(sizeof(struct ex_super_block));

    return required;
}
```

### exdbg
It is used for introspection of data structures. It can now display superblock, inode and structure size information, and can display bitmap and inode data. Bitmap and inode data are written in binary format on stdout, so it is useful to display them with something like  `xxd`.

### libexfuse
A library that contains all the filesystem logic. It does not depend on `libfuse`, its interfaces can be found in` src/ex .h`. Its primary purpose is to be used in tests.

### exfuse
Wrapper around `libfuse`. It contains `main` from where it calls` fuse_main`, to which it passes `struct fuse operations`. The implementation is in ` src/wrapper.c` .

## Usage
#### Create a filesystem
```sh
$ ./exmkfs --device foo --create --inodes 1024 --log-level info
```
####  Display information about the superblock
```sh
$ ./exdbg --device foo
super:
    root = 40960
    magic = ffaacc
    device_size = 1077977088 (1.004GiB)
data_bitmap:
    head = 16
    last = 31
    address = 8192
    size = 32768
    allocated = 256
    maxitems = 262144
inode_bitmap:
    head = 64
    last = 0
    address = 4096
    size = 128
    allocated = 1
    maxitems = 1024
```
#### Display information about the inode
```
$ ./exdbg --device foo --inode 40960
inode:
    number: 0
    size: 2144
    magic: abcc
    address: 40960
    nlinks: 2
    mode: 40705 (dir other=r-x group=--- user=rwx)
    mtime: 1568462913.009054549
    atime: 1568462913.009054549
    ctime: 1568462913.009054549
    entries:
        name: '.', address: 40960
        name: '..', address: 40960
```

#### Display the inode data
```sh
$ ./exdbg --device foo --inode-data 40960 | xxd
00000000: 00a0 0000 0000 0000 de00 2e00 0000 0000  ................
00000010: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000020: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000030: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000040: 00a0 0000 0000 0000 de00 2e2e 0000 0000  ................
00000050: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000060: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000070: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000080: 6161 6161 6161 6161 6161 6161 6161 6161  aaaaaaaaaaaaaaaa
00000090: 6161 6161 6161 6161 6161 6161 6161 6161  aaaaaaaaaaaaaaaa
```
#### Display the bitmap data
```sh
$ ./exdbg --device foo --bitmap-data 64 | xxd
00000000: 0100 0000 0000 0000 0000 0000 0000 0000  ................
00000010: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000020: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000030: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000040: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000050: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000060: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000070: 0000 0000 0000 0000 0000 0000 0000 0000  ................
```


#### Mount the filesystem

```sh
mkdir mp
./exfuse -f --device foo mp
```

## Main structures
```c
struct ex_super_block {
    // address of root inode
    inode_address root;
    // size of a device
    size_t device_size;
    // data allocation bitmap
    struct ex_bitmap bitmap;
    // inode allocation bitmap
    struct ex_bitmap inode_bitmap;
    // magic number for fs checking
    uint32_t magic;
};
```
```c
struct ex_inode {
    size_t number;

    uint32_t mode;

    uint16_t magic;

    uid_t uid;
    gid_t gid;

    struct timespec mtime;
    struct timespec atime;
    struct timespec ctime;

    inode_address address;

    uint16_t nlinks;

    // it has two meanings:
    // - it is the size of the struc_ex_inode if it is a directory
    // - otherwise it is size of the data
    size_t size;

    block_address blocks[EX_DIRECT_BLOCKS];
};
```
```c
struct ex_dir_entry {
    inode_address address;
    uint8_t magic;
    uint8_t free;
    char name[EX_NAME_LEN];
};
```

```c
struct ex_bitmap {
    // address of this structure on disk
    size_t head;
    // size of the bitmap in bytes
    size_t size;
    // address of the bitmap
    size_t address;
    // maximum number of items in a bitmap
    size_t max_items;
    // number of allocated blocks
    size_t allocated;
    // last allocated bit
    size_t last;
};
```


