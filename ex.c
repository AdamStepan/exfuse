// allow indexing by {name, utime, ...} -> inode (store them on disk)
// - it must be disk-on structure
//   + it's usually hidden "directory" which is maintained by superblock
// - it must have a reasonable memory footprint
// - it must has efficient lookups
// - it must suppor duplicate entries
// - possibly use plain b trees with node size equals to {1024, 2048, ...}
#include <ex.h>

static int device_fd = -1;

struct ex_super_block *super_block = NULL;
struct ex_inode *root = NULL;

static void *ex_malloc(size_t size) {

    void *memory = malloc(size);

    if(!memory) {
        err(errno, "malloc: amount=%lu", size);
    }

    memset(memory, '\0', size);

    return memory;
}

int ex_get_device_fd(void) {
    return device_fd;
}

int ex_device_create(const char *name, size_t size) {

    if(access(name, F_OK) != -1) {
        return 0;
    }

    int fd = open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if(fd == -1) {
        err(errno, "%s", name);
    }

    ftruncate(fd, size);

    close(fd);

    return 1;
}

int ex_device_open(const char *device_name) {

    device_fd = open(device_name, O_RDWR);

    if(device_fd == -1) {
        err(errno, "%s", device_name);
    }

    info("device is open: fd=%d", device_fd);

    return device_fd;
}

void *ex_read_device(size_t off, size_t amount) {

    int fd = ex_get_device_fd();

    void *data = ex_malloc(amount);

    off_t offset = lseek(fd, off, SEEK_SET);

    if(offset != off) {
        err(errno, "ex_read_device: offset=%lu, off=%lu", offset, off);
    }

    size_t readed = read(fd, data, amount);

    if(readed != amount) {
        err(errno, "ex_read_device: readed=%lu, amount=%lu", readed, amount);
    }

    return data;
}

void ex_write_device(size_t off, const char *data, size_t amount) {

    int fd = ex_get_device_fd();

    off_t offset = lseek(fd, off, SEEK_SET);

    if(offset != off) {
        err(errno, "ex_write_device: offset=%lu, off=%lu", offset, off);
    }

    size_t written = write(fd, data, amount);

    if(written != amount) {
        err(errno, "written=%lu, amount=%lu", written, amount);
    }
}

block_address ex_allocate_block(void) {

    char *bitmap = ex_read_device(super_block->bitmap,
                                  super_block->bitmap_size);
    size_t free_block_pos = 0;

    // go through all bits in bitmap, find first free bit
    for(size_t i = 0; i < super_block->bitmap_size; i++) {
        for(uint8_t bit = 0; bit < 8; bit++) {
            // n = (8 * i) + bit, nth block is occupied
            if(bitmap[i] & (1 << bit)) {
                continue;
            }

            // flip bit, compute absolute btt position
            bitmap[i] |= (1 << bit);
            free_block_pos = (8 * i) + bit;

            goto finded;
        }
    }

    info("unable to find free block");

    free(bitmap);
    return -1;
finded:
    info("found free block: position=%lu", free_block_pos);

    // XXX: we flip only one bit, we don't need to the rewrite whole bitmap
    ex_write_device(super_block->bitmap, bitmap, super_block->bitmap_size);
    free(bitmap);

    // compute block position
    return (super_block->bitmap + super_block->bitmap_size) +
        free_block_pos * EX_BLOCK_SIZE;
}

void ex_print_inode(const struct ex_inode *inode) {

    info("{.size=%ld, .name=%s, .magic=%x, .address=%lu, .mode=%o}",
            inode->size, inode->name, inode->magic, inode->address, inode->mode);

#ifdef VERBOSE
    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {
        info("block[%lu] = %ld", i, inode->blocks[i]);
    }
#endif

}

void ex_inode_allocate_blocks(struct ex_inode *inode) {

   static const char FREE_BLOCK[EX_BLOCK_SIZE] = {'a'};

   for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        block_address addr = ex_allocate_block();

        inode->blocks[i] = addr;

        ex_write_device(addr, FREE_BLOCK, sizeof(FREE_BLOCK));
    }
}

struct ex_inode *ex_inode_create(char *name, uint16_t mode) {

    inode_address address = ex_allocate_block();
    struct ex_inode *inode = ex_malloc(sizeof(struct ex_inode));

    inode->mode = mode;
    inode->magic = EX_INODE_MAGIC1;
    inode->parent_inode = 0;
    inode->mtime = time(0);
    inode->address = address;
    strcpy(inode->name, name);

    if(mode & S_IFDIR) {
        inode->size = sizeof(struct ex_inode);
    } else {
        inode->size = 0;
    }

    ex_inode_allocate_blocks(inode);

    ex_write_device(inode->address, (void *)inode, sizeof(struct ex_inode));

    return inode;
}

static void add_initial_structure(void) {
    struct ex_inode *a = ex_inode_create("a", S_IRWXU | S_IFDIR);
    struct ex_inode *b = ex_inode_create("b", S_IRWXU | S_IFREG);

    ex_dir_set_inode(root, a);
    ex_print_inode(a);

    ex_dir_set_inode(root, b);
    ex_inode_write(b, 0, "xxx", 3);

    free(a);
    free(b);
}

void ex_device_populate(size_t size) {
    // device
    // | super_block | bitmap | root | blocks

    info("populating device: size=%lu, blocks=%lu", size, size / EX_BLOCK_SIZE);
    info("super_block: block end=%lu", sizeof(struct ex_super_block));

    // every byte in bitmap represent 8 pages
    size_t bitmap_size = size / (EX_BLOCK_SIZE * 8);

    info("super_block: bitmap size=%lu", bitmap_size);

    size_t bitmap_addr = sizeof(struct ex_super_block);
    size_t root_addr = bitmap_addr + bitmap_size;

    info("super_block: bitmap addr=%lu", bitmap_addr);
    info("super_block: root_addr=%lu", root_addr);

    super_block = ex_malloc(sizeof(struct ex_super_block));

    *super_block = (struct ex_super_block){
        .root = root_addr,
        .device_size = size,
        .bitmap = bitmap_addr,
        .bitmap_size = bitmap_size
    };

    int fd = ex_get_device_fd();
    write(fd, super_block, sizeof(struct ex_super_block));

    info("super_block: written");

    root = ex_inode_create("/", S_IRWXU | S_IFDIR);
    root->parent_inode = root_addr;

    add_initial_structure();
}

struct ex_inode *ex_inode_load(inode_address ino_addr) {

    struct ex_inode *ino = ex_read_device(ino_addr, sizeof(struct ex_inode));
    assert(ino->magic == EX_INODE_MAGIC1);

    return ino;
}

void ex_print_super_block(const struct ex_super_block *block) {
    info("{.root=%lu, .device_size=%lu, bitmap=%lu, bitmap_size=%lu",
            block->root, block->device_size, block->bitmap, block->bitmap_size);
}

struct ex_inode *ex_dir_load_inode(struct ex_inode *ino, const char *name) {

    inode_address addr = ex_dir_get_inode(ino, name);

    if(!addr) {
        return NULL;
    }

    return ex_inode_load(addr);
}

void ex_device_load(void) {

    info("loading device");

    super_block = ex_read_device(0, sizeof(struct ex_super_block));
    ex_print_super_block(super_block);

    info("loading root");

    root = ex_inode_load(super_block->root);
    ex_print_inode(root);
}

void ex_init(void) {

    int created = ex_device_create(EX_DEVICE, EX_BLOCK_SIZE * EX_BLOCK_SIZE);
    info("device created: %d", created);

    ex_device_open(EX_DEVICE);

    if(created) {
        ex_device_populate(EX_BLOCK_SIZE * EX_BLOCK_SIZE);
    } else {
        ex_device_load();
    }
}

void ex_deinit(void) {

    info("deinitialize");

    if(device_fd != -1) {
        info("closing device: %i", device_fd);
        close(device_fd);
    }

    if(super_block) {
        info("freeing super_block");
        free(super_block);
    }

    if(root) {
        info("freeing root");
        free(root);
    }
}

void ex_print_struct_sizes(void) {
    info("ex_super_block: %lu", sizeof(struct ex_super_block));
    info("ex_inode: %lu", sizeof(struct ex_inode));
    info("ex_dir_entry: %lu", sizeof(struct ex_dir_entry));
}

ex_block *ex_block_read(block_address addr) {

    int fd = ex_get_device_fd();

    ex_block *block = ex_malloc(sizeof(ex_block));
    size_t readed = read(fd, block, sizeof(ex_block));

    assert(readed == sizeof(ex_block));

    return block;
}

inode_address ex_dir_set_inode(struct ex_inode *dir, struct ex_inode *ino) {

    size_t entry_address = 0;
    struct ex_dir_entry *entry = NULL;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, ientry) {

            entry = ientry;

            info("free=%i, magic=%i", entry->free, entry->magic);

            if(!entry->free) {
                continue;
            }

            info("finded: block=%ld, i=%ld", block_no, ientry_no);

            entry_address = block_addr + (ientry_no * sizeof(struct ex_dir_entry));
            goto update_entry;

        }
    }

    info("unable to find space for inode");
    return 0;

update_entry:

    entry->free = 0;
    entry->magic = EX_DIR_MAGIC1;
    strcpy(entry->name, ino->name);
    entry->address = ino->address;

    info("entry address=%ld", entry_address);

    ex_write_device(entry_address, (void *)entry, sizeof(struct ex_dir_entry));

    info("update inode parent: ino=%ld, parent=%ld", ino->address, dir->address);

    ino->parent_inode = dir->address;
    ex_write_device(ino->address, (void *)ino, sizeof(struct ex_inode));

    free(block);

    return entry_address;
}

void ex_deallocate_block(block_address address) {

}

void ex_inode_deallocate_blocks(struct ex_inode *inode) {

}

struct ex_inode *ex_dir_remove_inode(struct ex_inode *dir, const char *name) {

    struct ex_inode *inode = NULL;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            size_t entry_address = block_addr + (entry_no * sizeof(struct ex_dir_entry));

            if(entry->free || strcmp(entry->name, name) != 0) {
                continue;
            }

            info("entry {.name=%s, .iaddress=%lu}", entry->name, entry->address);

            entry->free = 1;

            ex_write_device(entry_address, (void *)entry, sizeof(struct ex_dir_entry));

            inode = ex_inode_load(entry->address);
            ex_inode_deallocate_blocks(inode);

            goto done;
        }
    }

    info("unable to remove inode: %s", name);

done:
    if(block) {
        free(block);
    }
    return inode;
}

struct ex_inode **ex_dir_get_inodes(struct ex_inode *ino) {

    assert(ino->mode & S_IFDIR);

    struct ex_inode **inodes = ex_malloc(sizeof(struct ex_inode *) * 16);
    size_t inode_no = 0;

    foreach_inode_block(ino, block) {
        foreach_block_entry(block, entry) {

            if(entry->free || entry->magic != EX_DIR_MAGIC1) {
                continue;
            }

            inodes[inode_no++] = ex_inode_load(entry->address);
        }
    }

    return inodes;
}

inode_address ex_dir_get_inode(struct ex_inode *ino, const char *name) {

    inode_address address = 0;

    foreach_inode_block(ino, block) {
        foreach_block_entry(block, entry) {

            if(entry->free || entry->magic != EX_DIR_MAGIC1) {
                continue;
            }

            if(strcmp(entry->name, name) == 0) {
                address = entry->address;
                goto finded;
            }
        }
    }

    info("inode=%ld does not contain=%s", ino->address, name);

    return 0;

finded:
    free(block);
    return address;
}

void ex_inode_write(struct ex_inode *ino, size_t off, const char *data, size_t amount) {

    info("off=%lu, amount=%lu", off, amount);

    if(off + amount > ino->size) {
        ino->size += (off + amount) - ino->size;
    }

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    assert(start_block_idx < EX_DIRECT_BLOCKS);

    block_address addr = ino->blocks[start_block_idx];

    ex_write_device(ino->address, (void *)ino, sizeof(struct ex_inode));
    ex_write_device(addr + start_block_off, data, amount);

#ifdef VERBOSE
    info("write: ino={%s, %ld}, data=%s", ino->name, ino->address, data);
#endif

}

char *ex_inode_read(struct ex_inode *ino, size_t off, size_t amount) {

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    assert(start_block_idx < EX_DIRECT_BLOCKS);

    char *block = ex_read_device(
        ino->blocks[start_block_idx] + start_block_off,
        amount
    );

    char *block_copy = ex_malloc((amount - start_block_off + 1) * sizeof(char));

    memcpy(block_copy, block, (amount - start_block_off) * sizeof(char));
    block_copy[amount - start_block_off] = '\0';

    free(block);

    return block_copy;
};

#ifdef _MAIN
int main(int argc, char** argv) {
    ex_print_struct_sizes();
    ex_init();
    ex_deinit();
}
#endif
