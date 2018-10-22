#include <inode.h>

struct ex_inode *root = NULL;

void ex_root_write(void) {

    assert(super_block);

    root = ex_inode_create(S_IRWXU | S_IFDIR);
    ex_inode_fill_dir(root);

    root->parent_inode = super_block->root;
    ex_inode_flush(root);
}

void ex_root_load(void) {

    assert(super_block);

    info("loading root");
    root = ex_inode_load(super_block->root);
}

int ex_inode_allocate_blocks(struct ex_inode *inode) {

    info("allocating blocks");

    // TODO: initialize it only once
    static char FREE_BLOCK[EX_BLOCK_SIZE];
    memset(FREE_BLOCK, 'a', EX_BLOCK_SIZE);

    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        inode->blocks[i] = ex_super_allocate_block();

        // we are unable to allocate next block, we should deallocate all
        // already allocated blocks
        if(inode->blocks[i] == -1) {
            ex_inode_deallocate_blocks(inode);
            return 0;
        }

        ex_device_write(inode->blocks[i], FREE_BLOCK, sizeof(FREE_BLOCK));
    }

    return 1;
}

void ex_inode_deallocate_blocks(struct ex_inode *inode) {

    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        if(inode->blocks[i] == -1) {
            break;
        }

        ex_super_deallocate_block(inode->blocks[i]);
    }

    ex_super_deallocate_block(inode->address);
}

void ex_inode_free(struct ex_inode *inode) {
    free(inode);
}

void ex_inode_print(const struct ex_inode *inode) {

    info("{.size=%ld, magic=%x, .address=%lu, .mode=%o}",
            inode->size, inode->magic, inode->address, inode->mode);

#ifdef VERBOSE
    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {
        info("block[%lu] = %ld", i, inode->blocks[i]);
    }
#endif
}

void ex_inode_flush(const struct ex_inode *inode) {
    ex_device_write(inode->address, (void *)inode, sizeof(struct ex_inode));
}

struct ex_inode *ex_copy_inode(const struct ex_inode *inode) {

    struct ex_inode *copy = ex_malloc(sizeof(struct ex_inode));

    copy->mode = inode->mode;
    copy->magic = inode->magic;
    copy->parent_inode = inode->parent_inode;

    copy->mtime = inode->mtime;
    copy->ctime = inode->ctime;
    copy->atime = inode->atime;

    copy->address = inode->address;
    copy->size = inode->size;
    copy->nlinks = inode->nlinks;

    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {
        copy->blocks[i] = inode->blocks[i];
    }

    return copy;
}

void ex_inode_fill_dir(struct ex_inode *inode) {

    struct ex_inode *parent = NULL;

    if(inode->parent_inode) {
        parent = ex_inode_load(inode->parent_inode);
    } else {
        parent = inode;
    }

    ex_inode_set(inode, ".", inode);
    ex_inode_set(inode, "..", parent);

    ex_inode_flush(inode);
}

struct ex_inode *ex_inode_create(uint16_t mode) {

    info("allocating inode block");

    inode_address address = ex_super_allocate_block();
    struct ex_inode *inode = ex_malloc(sizeof(struct ex_inode));

    inode->mode = mode;
    inode->magic = EX_INODE_MAGIC1;
    inode->parent_inode = 0;

    ex_update_time_ns(&(inode->mtime));
    inode->ctime = inode->mtime;
    inode->atime = inode->mtime;

    inode->address = address;

    if(mode & S_IFDIR) {
        inode->size = sizeof(struct ex_inode);
        inode->nlinks = 2;
    } else {
        inode->size = 0;
        inode->nlinks = 1;
    }

    if(!ex_inode_allocate_blocks(inode)) {
        return 0;
    }

    ex_device_write(inode->address, (void *)inode, sizeof(struct ex_inode));

    return inode;
}

struct ex_inode *ex_inode_set(struct ex_inode *dir, const char *name, struct ex_inode *ino) {

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

    info("unable to find space for inode in dirinode");
    return NULL;

update_entry:

    entry->free = 0;
    entry->magic = EX_DIR_MAGIC1;
    strcpy(entry->name, name);
    entry->address = ino->address;

    info("entry address=%ld", entry_address);

    ex_device_write(entry_address, (void *)entry, sizeof(struct ex_dir_entry));

    info("update inode parent: ino=%ld, parent=%ld", ino->address, dir->address);

    // XXX: this is probably bad idea, due to hardlinks
    ino->parent_inode = dir->address;
    ex_device_write(ino->address, (void *)ino, sizeof(struct ex_inode));

    free(block);

    return ino;
}

// XXX: check that all calls check return value
struct ex_inode *ex_inode_load(inode_address ino_addr) {

    struct ex_inode *ino = ex_device_read(ino_addr, sizeof(struct ex_inode));

    if(ino->magic != EX_INODE_MAGIC1) {
        ex_inode_free(ino);
        return NULL;
    }

    return ino;
}

struct ex_inode *ex_inode_find(struct ex_path *path) {

    if(!strcmp("/", path->basename) && path->ncomponents == 0) {
        return ex_copy_inode(root);
    }

    struct ex_inode *searched = NULL, *curdir = root;
    size_t n = 0;

    while(curdir) {

        if(!path->components[n]) {
            goto not_found;
        }

        // TODO: free inode
        searched = ex_inode_get(curdir, path->components[n]);

        if(!searched) {
            goto not_found;
        }

        // we found our file
        if(n + 1 == path->ncomponents) {
            debug("found: %lu/%lu", n, path->ncomponents);
            goto found;
        }

        if(!(searched->mode & S_IFDIR)) {
            debug("component: %lu is not directory at: %s", curdir->address, path->components[n]);
            goto not_found;
        }

        n++;

        curdir = searched;

        debug("looking for: %s in %lu", path->components[n], curdir->address);
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

    debug("inode=%ld does not contain=%s", dir->address, name);

    return 0;

finded:
    free(block);

    return inode;
}


static size_t __ex_dir_entries(struct ex_inode *inode) {
    // check if directory is empty
    struct ex_dir_entry **entries = ex_inode_get_all(inode);
    size_t entries_count = 0;

    for(; entries[entries_count]; entries_count++);

    for(size_t i = 0; i < entries_count; i++) {
        free(entries[entries_count]);
    }

    free(entries);

    info("inode=%lu, entries_count=%lu", inode->address, entries_count);

    return entries_count;
}

static int __ex_inode_unlink(struct ex_inode *inode) {

    // not a directory, we can deallocate blocks
    if (!(inode->mode & S_IFDIR)) {
        info("not a directory");
        goto unlink_inode;

    }

    // we cannot remove directory if it has more than two entries ('.', '..')
    if(__ex_dir_entries(inode) > 2) {
        return 0;
    } else {
        goto unlink_inode;
    }

unlink_inode:
    return 1;
}


struct ex_inode *ex_inode_unlink(struct ex_inode *dir, const char *name) {

    struct ex_inode *inode = NULL;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            size_t entry_address = block_addr + (entry_no * sizeof(struct ex_dir_entry));

            if(entry->free || strcmp(entry->name, name) != 0) {
                continue;
            }

            info("entry {.name=%s, .iaddress=%lu}", entry->name, entry->address);

            inode = ex_inode_load(entry->address);

            if(!__ex_inode_unlink(inode)) {
                info("unable to unlink");
                goto fail;
            }

            if(inode->mode & S_IFDIR) {
                inode->nlinks -= 2;
            } else {
                inode->nlinks -= 1;
            }

            if(!inode->nlinks) {
                ex_inode_deallocate_blocks(inode);
            }

            ex_inode_flush(inode);

            entry->free = 1;
            ex_device_write(entry_address, (void *)entry, sizeof(struct ex_dir_entry));

            goto done;
        }
    }

    info("unable to remove inode: %s", name);

fail:
    ex_inode_free(inode);
    return NULL;

done:
    if(block) {
        free(block);
    }

    return inode;
}

struct ex_dir_entry *ex_dir_entry_copy(const struct ex_dir_entry *entry) {

    struct ex_dir_entry *new_entry = ex_malloc(sizeof(struct ex_dir_entry));

    new_entry->address = entry->address;
    new_entry->magic = entry->magic;
    new_entry->free = entry->free;
    strcpy(new_entry->name, entry->name);

    return new_entry;
}

void ex_dir_entry_free(struct ex_dir_entry *entry) {
    free(entry);
}

struct ex_dir_entry **ex_inode_get_all(struct ex_inode *ino) {

    if(!(ino->mode & S_IFDIR)) {
        warning("inode on %lu is not directory", ino->address);
        return NULL;
    }

    size_t max_direntries = 16, dir_entry_no = 0;
    struct ex_dir_entry **entries = ex_malloc(sizeof(struct ex_dir_entry *) * max_direntries);

    foreach_inode_block(ino, block) {
        foreach_block_entry(block, entry) {

            if(entry->free) {
                continue;
            }

            // no more entries are present in current block
            if(entry->magic != EX_DIR_MAGIC1) {
                break;
            }

            entries[dir_entry_no++] = ex_dir_entry_copy(entry);
            entries[dir_entry_no] = NULL;

            if(dir_entry_no >= max_direntries) {
                max_direntries <<= 1;
                entries = ex_realloc(entries, sizeof(struct ex_dir_entry *) * max_direntries);
            }
        }
    }

    return entries;
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
