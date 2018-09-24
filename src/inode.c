#include <inode.h>

struct ex_inode *root = NULL;

void ex_root_write(void) {

    assert(super_block);
    root = ex_inode_create("/", S_IRWXU | S_IFDIR);
    root->parent_inode = super_block->root;

    ex_device_write(super_block->root,
                    (char *)root,
                    sizeof(struct ex_inode));
}

void ex_root_load(void) {

    assert(super_block);

    info("loading root");
    root = ex_inode_load(super_block->root);
}

void ex_inode_allocate_blocks(struct ex_inode *inode) {

   static char FREE_BLOCK[EX_BLOCK_SIZE];
   memset(FREE_BLOCK, 'a', EX_BLOCK_SIZE);

   for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        block_address addr = ex_super_allocate_block();

        inode->blocks[i] = addr;

        ex_device_write(addr, FREE_BLOCK, sizeof(FREE_BLOCK));
    }
}

void ex_inode_deallocate_blocks(struct ex_inode *inode) {
    warning("unimplemented");
}

void ex_inode_free(struct ex_inode *inode) {
    free(inode);
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

struct ex_inode *ex_copy_inode(const struct ex_inode *inode) {

    struct ex_inode *copy = ex_malloc(sizeof(struct ex_inode));

    copy->mode = inode->mode;
    copy->magic = inode->magic;
    copy->parent_inode = inode->parent_inode;
    copy->mtime = inode->mtime;
    copy->address = inode->address;
    copy->size = inode->size;

    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {
        copy->blocks[i] = inode->blocks[i];
    }

    strcpy(copy->name, inode->name);

    return copy;
}

struct ex_inode *ex_inode_create(char *name, uint16_t mode) {

    inode_address address = ex_super_allocate_block();
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

    ex_device_write(inode->address, (void *)inode, sizeof(struct ex_inode));

    return inode;
}

struct ex_inode *ex_inode_set(struct ex_inode *dir, struct ex_inode *ino) {

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
    return NULL;

update_entry:

    entry->free = 0;
    entry->magic = EX_DIR_MAGIC1;
    strcpy(entry->name, ino->name);
    entry->address = ino->address;

    info("entry address=%ld", entry_address);

    ex_device_write(entry_address, (void *)entry, sizeof(struct ex_dir_entry));

    info("update inode parent: ino=%ld, parent=%ld", ino->address, dir->address);

    ino->parent_inode = dir->address;
    ex_device_write(ino->address, (void *)ino, sizeof(struct ex_inode));

    free(block);

    return ino;
}

struct ex_inode *ex_inode_load(inode_address ino_addr) {

    struct ex_inode *ino = ex_device_read(ino_addr, sizeof(struct ex_inode));
    assert(ino->magic == EX_INODE_MAGIC1);

    return ino;
}

struct ex_inode *ex_inode_find(struct ex_inode *dir, struct ex_path *path) {

    if(!strcmp(dir->name, path->basename) && path->ncomponents == 0) {
        return ex_copy_inode(dir);
    }

    struct ex_inode *searched = NULL, *curdir = dir;
    size_t n = 0;

    while(curdir) {

        if(!path->components[n]) {
            goto not_found;
        }

        // TODO: free inode
        searched = ex_inode_get(curdir, path->components[n]);

        if(!searched) {
            debug("dir: %s does not contain: %s", curdir->name, path->components[n]);
            goto not_found;
        }

        // we found our file
        if(n + 1 == path->ncomponents) {
            info("found: %lu/%lu", n, path->ncomponents);
            goto found;
        }

        if(!(searched->mode & S_IFDIR)) {
            debug("component: %s is not directory at: %s", curdir->name, path->components[n]);
            goto not_found;
        }

        n++;

        curdir = searched;

        debug("looking for: %s in %s", path->components[n], curdir->name);
    }

found:
    return searched;

not_found:
    return NULL;
}

struct ex_inode *ex_inode_get(struct ex_inode *dir, const char *name) {

    struct ex_inode *inode = NULL;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            if(entry->free || entry->magic != EX_DIR_MAGIC1) {
                continue;
            }

            if(!strcmp(entry->name, name)) {
                inode = ex_inode_load(entry->address);
                goto finded;
            }
        }
    }

    info("inode=%ld does not contain=%s", dir->address, name);

    return 0;

finded:
    free(block);

    return inode;
}
struct ex_inode *ex_inode_remove(struct ex_inode *dir, const char *name) {

    struct ex_inode *inode = NULL;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            size_t entry_address = block_addr + (entry_no * sizeof(struct ex_dir_entry));

            if(entry->free || strcmp(entry->name, name) != 0) {
                continue;
            }

            info("entry {.name=%s, .iaddress=%lu}", entry->name, entry->address);

            entry->free = 1;

            ex_device_write(entry_address, (void *)entry, sizeof(struct ex_dir_entry));

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

struct ex_inode **ex_inode_get_all(struct ex_inode *ino) {

    if(!(ino->mode & S_IFDIR)) {
        warning("inode on %lu is not directory", ino->address);
        return NULL;
    }

    size_t max_inodes = 16, inode_no = 0;
    struct ex_inode **inodes = ex_malloc(sizeof(struct ex_inode *) * max_inodes);

    foreach_inode_block(ino, block) {
        foreach_block_entry(block, entry) {

            if(entry->free) {
                continue;
            }

            // no more entries are present in current block
            if(entry->magic != EX_DIR_MAGIC1) {
                break;
            }

            inodes[inode_no++] = ex_inode_load(entry->address);
            inodes[inode_no] = NULL;

            if(inode_no >= max_inodes) {
                max_inodes <<= 1;
                inodes = ex_realloc(inodes, sizeof(struct ex_inode *) * max_inodes);
            }
        }
    }

    return inodes;
}

ssize_t ex_inode_write(struct ex_inode *ino, size_t off, const char *data, size_t amount) {

    info("off=%lu, amount=%lu", off, amount);

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    // TODO: handle situation of writing after last allocated block
    assert(start_block_idx < EX_DIRECT_BLOCKS);

    if(off + amount > ino->size) {
        ino->size += (off + amount) - ino->size;
    }

    block_address addr = ino->blocks[start_block_idx];

    ex_device_write(ino->address, (void *)ino, sizeof(struct ex_inode));
    ex_device_write(addr + start_block_off, data, amount);

    return (ssize_t)amount;
}

char *ex_inode_read(struct ex_inode *ino, size_t off, size_t amount) {

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    assert(start_block_idx < EX_DIRECT_BLOCKS);

    char *block = ex_device_read(
        ino->blocks[start_block_idx] + start_block_off,
        amount
    );

    char *block_copy = ex_malloc((amount - start_block_off + 1) * sizeof(char));

    memcpy(block_copy, block, (amount - start_block_off) * sizeof(char));
    block_copy[amount - start_block_off] = '\0';

    free(block);

    return block_copy;
}
