#include <inode.h>

struct ex_inode *root = NULL;

void ex_root_write(void) {

    assert(super_block);

    root = ex_inode_create(S_IRWXU | S_IXOTH | S_IROTH | S_IFDIR);

    if(!root) {
        fatal("unable to create root inode");
    }

    super_block->root = root->address;

    ex_inode_flush(root);
    ex_inode_fill_dir(root, root);

    ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));
}

void ex_root_load(void) {

    assert(super_block);

    info("loading root");
    root = ex_inode_load(super_block->root);

    if(!root) {
        fatal("unable to load root from %lu", super_block->root);
    }
}

int ex_inode_allocate_blocks(struct ex_inode *inode) {

    info("allocating blocks");

    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        inode->blocks[i] = ex_super_allocate_block();

        // we are unable to allocate next block, we should deallocate all
        // already allocated blocks
        if(inode->blocks[i] == EX_BLOCK_INVALID_ADDRESS) {
            warning("failing to allocate nth (%lu) block", i);
            ex_inode_deallocate_blocks(inode);
            return 0;
        }

    }

    return 1;
}

void ex_inode_deallocate_blocks(struct ex_inode *inode) {

    for(size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        if(inode->blocks[i] == EX_BLOCK_INVALID_ADDRESS) {
            break;
        }

        ex_super_deallocate_block(inode->blocks[i]);
    }

    ex_super_deallocate_inode_block(inode->number);
}

void ex_inode_free(struct ex_inode *inode) {
    free(inode);
}

void ex_inode_print(const struct ex_inode *inode) {

    info("number: %lu", inode->number);
    info("mode: %o", inode->mode);
    info("magic: %x", inode->magic);
    info("address: %lu", inode->address);
    info("links: %u", inode->nlinks);
    info("size: %lu", inode->size);

    info("mtime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    info("atime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    info("ctime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
}

void ex_inode_flush(const struct ex_inode *inode) {
    ex_device_write(inode->address, (void *)inode, sizeof(struct ex_inode));
}

struct ex_inode *ex_copy_inode(const struct ex_inode *inode) {

    struct ex_inode *copy = ex_malloc(sizeof(struct ex_inode));

    copy->number = inode->number;
    copy->mode = inode->mode;
    copy->magic = inode->magic;

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

void ex_inode_fill_dir(struct ex_inode *inode, struct ex_inode *parent) {

    ex_inode_set(inode, ".", inode);
    ex_inode_set(inode, "..", parent);

    ex_inode_flush(inode);
}

struct ex_inode *ex_inode_create(uint16_t mode) {

    struct ex_inode_block block = ex_super_allocate_inode_block();

    if(block.address == EX_BLOCK_INVALID_ADDRESS) {
        warning("inode block allocation failed");
        goto inode_creation_failed;
    }

    struct ex_inode *inode = ex_malloc(sizeof(struct ex_inode));

    if(!inode) {
        warning("unable to allocate memory for inode");
        goto free_inode_block;
    }

    inode->number = block.id;
    inode->address = block.address;
    inode->mode = mode;
    inode->magic = EX_INODE_MAGIC1;

    ex_update_time_ns(&(inode->mtime));
    inode->ctime = inode->mtime;
    inode->atime = inode->mtime;

    if(mode & S_IFDIR) {
        inode->size = sizeof(struct ex_inode);
        inode->nlinks = 2;
    } else {
        inode->size = 0;
        inode->nlinks = 1;
    }

    if(!ex_inode_allocate_blocks(inode)) {
        goto free_inode_block;
    }

    ex_inode_flush(inode);

    return inode;

free_inode_block:
    ex_super_deallocate_inode_block(block.address);

inode_creation_failed:
    return NULL;
}

size_t ex_inode_find_free_entry_address(struct ex_inode *dir) {

    size_t address = 0;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            debug("free=%i, magic=%i", entry->free, entry->magic);

            if(!entry->free) {
                continue;
            }

            debug("finded: block=%ld, i=%ld", block_iterator.block_number, entry_no);

            address = block.address + (entry_no * sizeof(struct ex_dir_entry));

            goto found;
        }
    }

found:
    foreach_inode_block_cleanup(dir, block);

    return address;
}

void ex_inode_entry_update(size_t address, const char *name, size_t inode_address, int free) {

    struct ex_dir_entry entry;

    entry.free = free;
    entry.address = inode_address;
    entry.magic = EX_DIR_MAGIC1;

    strncpy(entry.name, name, EX_NAME_LEN);

    debug("updating dir entry: address=%ld, name=%s", address, name);

    ex_device_write(address, (void *)&entry, sizeof(entry));
}

struct ex_inode *ex_inode_set(struct ex_inode *dir, const char *name, struct ex_inode *inode) {

    size_t entry_address = ex_inode_find_free_entry_address(dir);

    if(!entry_address) {
        info("unable to find space for inode in dirinode");
        return NULL;
    }

    ex_inode_entry_update(entry_address, name, inode->address, 0);
    ex_inode_flush(inode);

    return inode;
}

struct ex_inode *ex_inode_load_unsafe(inode_address address) {
    return ex_device_read(address, sizeof(struct ex_inode));
}

struct ex_inode *ex_inode_load(inode_address address) {

    struct ex_inode *inode = ex_device_read(address, sizeof(struct ex_inode));

    if(inode->magic != EX_INODE_MAGIC1) {
        warning("inode (%lu) has bad magic (%x)", address, inode->magic);
        ex_inode_free(inode);
        return NULL;
    }

    return inode;
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

            if(strcmp(entry->name, name)) {
                continue;
            }

            inode = ex_inode_load(entry->address);

            if(!inode) {
                error("unable to load inode at: %lu", entry->address);
            }

            goto finded;
        }
    }

    debug("inode=%ld does not contain=%s", dir->address, name);

finded:
    foreach_inode_block_cleanup(dir, block);
    return inode;
}


static size_t __ex_dir_entries_count(struct ex_inode *inode) {
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
    if(__ex_dir_entries_count(inode) > 2) {
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

            size_t entry_address = block.address + (entry_no * sizeof(struct ex_dir_entry));

            if(entry->free || strcmp(entry->name, name) != 0) {
                continue;
            }

            info("entry {.name=%s, .iaddress=%lu}", entry->name, entry->address);

            inode = ex_inode_load(entry->address);

            if(!inode) {
                error("unable to load inode from %lu", entry->address);
                goto done;
            }

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
    inode = NULL;

done:
    foreach_inode_block_cleanup(dir, block);
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

struct ex_dir_entry **ex_inode_get_all(struct ex_inode *dir) {

    if(!(dir->mode & S_IFDIR)) {
        warning("inode on %lu is not directory", dir->address);
        return NULL;
    }

    size_t max_direntries = 16, dir_entry_no = 0;
    struct ex_dir_entry **entries = ex_malloc(sizeof(struct ex_dir_entry *) * max_direntries);

    foreach_inode_block(dir, block) {
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

    foreach_inode_block_cleanup(dir, block);

    return entries;
}

ssize_t ex_inode_write(struct ex_inode *ino, size_t off, const char *data, size_t amount) {

    info("off=%lu, amount=%lu", off, amount);

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    if(start_block_idx >= EX_DIRECT_BLOCKS || amount / EX_BLOCK_SIZE >= EX_DIRECT_BLOCKS) {
        return -1;
    }

    if(off + amount > ino->size) {
        ino->size += (off + amount) - ino->size;
    }

    block_address addr = ino->blocks[start_block_idx];

    ex_device_write(ino->address, (void *)ino, sizeof(struct ex_inode));
    ex_device_write(addr + start_block_off, data, amount);

    return (ssize_t)amount;
}

ssize_t ex_inode_read(struct ex_inode *ino, size_t off, char *buffer, size_t amount) {

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    if(start_block_idx >= EX_DIRECT_BLOCKS) {
        return 0;
    }

    size_t offset = ino->blocks[start_block_idx] + start_block_off;

    return ex_device_read_to_buffer(buffer, offset, amount);
}

struct ex_inode_block ex_inode_block_iterate(struct ex_inode *inode, struct ex_block_iterator *it) {

    // not first iteration
    if(it->last_block.data) {
        free(it->last_block.data);
        it->block_number++;
    }

    struct ex_inode_block block = {
        .data = NULL,
        .id = EX_BLOCK_INVALID_ID,
        .address = EX_BLOCK_INVALID_ADDRESS
    };

    if(it->block_number >= EX_DIRECT_BLOCKS) {
        goto done;
    }

    block.address = inode->blocks[it->block_number];
    // XXX: add buffer into ex_block_iterator and use ex_device_read_to_buffer
    //      handle read error
    block.data = ex_device_read(block.address, EX_BLOCK_SIZE);

done:
    it->last_block = block;
    return block;
}
