#include "inode.h"
#include "errors.h"

const uint16_t EX_INODE_MAGIC1 = 0xabcc;
const uint8_t EX_DIR_MAGIC1 = 0xde;
const uint8_t EX_ENTRY_MAGIC1 = 0x61;

ex_status ex_root_create(void) {

    if (!super_block) {
        return SUPER_BLOCK_IS_NOT_LOADED;
    }

    mode_t mode = S_IRWXU | S_IXOTH | S_IROTH | S_IFDIR;
    struct ex_inode root;

    if (ex_inode_create(&root, mode, getgid(), getuid()) != OK) {
        error("unable to create root inode");
        return ROOT_INODE_CANNOT_BE_CREATED;
    }

    super_block->root = root.address;

    ex_inode_flush(&root);
    ex_inode_fill_dir(&root, &root);

    return ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));
}

void ex_inode_copy_noalloc(const struct ex_inode *src, struct ex_inode *dest) {

    dest->number = src->number;
    dest->mode = src->mode;
    dest->magic = src->magic;

    dest->mtime = src->mtime;
    dest->ctime = src->ctime;
    dest->atime = src->atime;

    dest->address = src->address;
    dest->size = src->size;
    dest->nlinks = src->nlinks;

    dest->gid = src->gid;
    dest->uid = src->uid;

    for (size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {
        dest->blocks[i] = src->blocks[i];
    }
}

ex_status ex_root_load(struct ex_inode *root) {

    ex_status status = OK;

    if (!super_block) {
        status = SUPER_BLOCK_IS_NOT_LOADED;
        goto done;
    }

    info("loading root from %zu", super_block->root);
    status = ex_inode_load(super_block->root, root);

    if (status != OK) {
        error("unable to load root from %lu", super_block->root);
        status = ROOT_CANNOT_BE_LOADED;
        goto done;
    }

done:
    return status;
}

static const size_t blocks_per_block = EX_BLOCK_SIZE / sizeof(size_t);

ex_status ex_inode_allocate_indirect_block(struct ex_inode *inode, size_t n) {

    debug("alllocating nth block: %zu", n);

    // TODO: add new kind of error
    size_t indirect_block_no = n;

    size_t parent_block_no = EX_DIRECT_BLOCKS + indirect_block_no / blocks_per_block;
    size_t block_off = indirect_block_no % blocks_per_block;

    // check if parent of the indirect block is allocated
    // TODO: move it to the separeate function
    if (!inode->blocks[parent_block_no].allocated) {

        debug("allocating parent inode: %zu", parent_block_no);

        size_t address = ex_super_allocate_block();
        // we were unable to allocate parent block, there is nothing
        // we can do
        if (address == EX_BLOCK_INVALID_ADDRESS) {
            warning("unable to allocate block: %zu for inode: %zu",
                    n, inode->number);
            return INODE_BLOCK_ALLOCATION_FAILED;
        }

        debug("parent inode is at: %zu", address);

        inode->blocks[parent_block_no].address = address;
        inode->blocks[parent_block_no].allocated = 1;
        inode->nblocks++;

        // clear the block space
        char empty[EX_BLOCK_SIZE] = {0};
        ex_device_write(address, (char *)empty, sizeof(empty));

        ex_inode_flush(inode);
    }

    // read parent block
    size_t parent_block_address = inode->blocks[parent_block_no].address;
    ssize_t readed = 0;
    struct ex_data_block parent_block[blocks_per_block];
    memset(parent_block, 0, sizeof(parent_block));

    if (ex_device_read_to_buffer(&readed, (char *)parent_block,
                                 parent_block_address, sizeof(parent_block)) != OK) {
        warning("unable to read parent block at: %zu", parent_block_address);
        return PARENT_BLOCK_CANNOT_BE_READED;
    }

    // allocate the block and write changes to the disk
    struct ex_data_block block = parent_block[block_off];

    if (block.allocated) {
        warning("indirect block: %zu is already allocated", n);
    } else {

        debug("allocating new indirect block");

        size_t address = ex_super_allocate_block();

        if (address == EX_BLOCK_INVALID_ADDRESS) {
            return INODE_BLOCK_ALLOCATION_FAILED;
        }

        debug("block %zu in parent %zu is at (%zu)", block_off,
                parent_block_no, address);

        parent_block[block_off].allocated = 1;
        parent_block[block_off].address = address;

        ex_device_write(parent_block_address,
                        (char *)parent_block,
                        sizeof(parent_block));

        inode->nblocks++;
        ex_inode_flush(inode);
    }
    return OK;
}

ex_status ex_inode_allocate_blocks(struct ex_inode *inode) {

    ex_status status = OK;

    debug("allocating blocks for inode (%lu)", inode->number);

    for (size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        struct ex_inode_block block;

        if ((status = ex_super_allocate_data_block(&block)) != OK) {
            warning("failing to allocate nth (%lu) block", i);
            goto done;
        }

        struct ex_data_block *block = &inode->blocks[i];

        inode->nblocks += 1;
        block->address = address;
        block->allocated = 1;
    }

    for (size_t i = EX_DIRECT_BLOCKS; i < EX_DIRECT_BLOCKS + EX_INDIRECT_BLOCKS; i++) {
        inode->blocks[i].allocated = 0;
        inode->blocks[i].address = 0;
    }

done:

    // we are unable to allocate next block, we should deallocate all
    // already allocated blocks
    if (status != OK) {
        ex_inode_deallocate_blocks(inode);
        status = INODE_BLOCK_ALLOCATION_FAILED;
    }

    return status;
}

void ex_inode_deallocate_blocks(struct ex_inode *inode) {

    for (size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        if (inode->blocks[i].address == EX_BLOCK_INVALID_ADDRESS) {
            break;
        }

        ex_super_deallocate_block(inode->blocks[i].address);
    }

    ex_super_deallocate_inode_block(inode->number);
}

void ex_inode_free(struct ex_inode *inode) { free(inode); }

void ex_inode_print(const struct ex_inode *inode) {

    info("number: %lu", inode->number);
    info("mode: %o", inode->mode);
    info("magic: %x", inode->magic);
    info("address: %lu", inode->address);
    info("links: %u", inode->nlinks);
    info("size: %lu", inode->size);
    info("nblocks: %lu", inode->nblocks);

    info("uid: %u", inode->uid);
    info("gid: %u", inode->gid);

    info("mtime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    info("atime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    info("ctime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
}

ex_status ex_inode_flush(const struct ex_inode *inode) {

    ex_status status = ex_device_write(inode->address, (void *)inode,
                                       sizeof(struct ex_inode));

    if (status != OK) {
        error("inode (%lu) flush failed", inode->number);
        status = INODE_FLUSH_FAILED;
    }

    return status;
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

    copy->gid = inode->gid;
    copy->uid = inode->uid;

    copy->nblocks = inode->nblocks;

    for (size_t i = 0; i < EX_DIRECT_BLOCKS + EX_INDIRECT_BLOCKS; i++) {
        copy->blocks[i] = inode->blocks[i];
    }

    return copy;
}

void ex_inode_fill_dir(struct ex_inode *inode, struct ex_inode *parent) {

    ex_inode_set(inode, ".", inode);
    ex_inode_set(inode, "..", parent);

    ex_inode_flush(inode);
}

ex_status ex_inode_create(struct ex_inode *inode, uint16_t mode, gid_t gid, uid_t uid) {

    struct ex_inode_block block;

    if (ex_super_allocate_inode_block(&block) != OK) {
        goto inode_creation_failed;
    }

    inode->number = block.id;
    inode->address = block.address;
    inode->mode = mode;
    inode->magic = EX_INODE_MAGIC1;

    ex_update_time_ns(&(inode->mtime));
    inode->ctime = inode->mtime;
    inode->atime = inode->mtime;

    inode->gid = gid;
    inode->uid = uid;

    if (mode & S_IFDIR) {
        inode->size = sizeof(struct ex_inode);
        inode->nlinks = 2;
    } else {
        inode->size = 0;
        inode->nlinks = 1;
    }

    inode->nblocks = 0;

    if (ex_inode_allocate_blocks(inode) != OK) {
        goto free_inode_block;
    }

    ex_inode_flush(inode);

    return OK;

free_inode_block:
    ex_super_deallocate_inode_block(block.address);

inode_creation_failed:

    error("unable to create an inode");

    return INODE_CREATION_FAILED;
}

size_t ex_inode_find_free_entry_address(struct ex_inode *dir) {

    debug("trying to find entry address in: %zu", dir->number);

    size_t address = 0;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            //debug("free=%i, magic=%x, blockaddr=%lu", entry.free, entry.magic,
            //      block.address);

            if (!entry.free && entry.magic != EX_ENTRY_MAGIC1) {
                continue;
            }

            debug("free finded: block=%ld, i=%ld", block_iterator.block_number,
                  entry_iterator.entry_number);

            address = block.address + (entry_iterator.entry_number *
                                       sizeof(struct ex_dir_entry));

            goto found;
        }
    }

found:
//    foreach_inode_block_cleanup(dir, block);

    return address;
}

void ex_inode_entry_update(size_t address, const char *name,
                           size_t inode_address, int free) {

    struct ex_dir_entry entry;

    entry.free = free;
    entry.address = inode_address;
    entry.magic = EX_DIR_MAGIC1;

    strncpy(entry.name, name, EX_NAME_LEN);

    debug("updating dir entry: address=%ld, name=%s", address, name);

    ex_device_write(address, (void *)&entry, sizeof(entry));
}

size_t next_indirect(struct ex_inode *inode) {
    if (inode->nblocks < EX_DIRECT_BLOCKS) {
        return 0;
    }

    size_t next = inode->nblocks - EX_DIRECT_BLOCKS;

    if (next == 0) { return next; }

    // subtract the amount of parent indirect blocks
    size_t parents = next / blocks_per_block + 1;

    return next - parents;
}

struct ex_inode *ex_inode_set(struct ex_inode *dir, const char *name,
                              struct ex_inode *inode) {

    // XXX: we should check if `name' is not present in a `dir`
    size_t entry_address = ex_inode_find_free_entry_address(dir);

    if (!entry_address) {

        info("unable to find space for inode in dirinode: %zu", dir->number);

        if (ex_inode_allocate_indirect_block(dir, next_indirect(dir)) != OK) {
            return NULL;
        }

        entry_address = ex_inode_find_free_entry_address(dir);

        if (!entry_address) {
            warning("unable to find free space after relocation");
            return NULL;
        }

    }

    ex_inode_entry_update(entry_address, name, inode->address, 0);
    ex_inode_flush(inode);

    return inode;
}

ex_status ex_inode_load(inode_address address, struct ex_inode *inode) {

    ex_status status = ex_device_read_to_buffer(NULL, (char *)inode, address,
            sizeof(struct ex_inode));

    if (status != OK) {
        warning("unable to read inode at (%zu)", address);
        goto error;
    }

    if (inode->magic != EX_INODE_MAGIC1) {
        warning("inode at (%zu) has bad magic (%x)", address, inode->magic);
        goto error;
    }

    return OK;
error:
    return INODE_LOAD_FAILED;
}

struct ex_inode *ex_inode_find(struct ex_path *path) {

    struct ex_inode root;

    if (ex_root_load(&root) != OK) {
        error("Unable to search for inode, because root cannot be loaded");
        return NULL;
    }

    if (!strcmp("/", path->basename) && path->ncomponents == 0) {
        return ex_copy_inode(&root);
    }

    struct ex_inode *searched = NULL, *curdir = &root;
    size_t n = 0;

    while (curdir) {

        if (!path->components[n]) {
            goto not_found;
        }

        searched = ex_inode_get(curdir, path->components[n]);

        if (!searched) {
            goto not_found;
        }

        // we found our file
        if (n + 1 == path->ncomponents) {
            debug("found: %lu/%lu", n, path->ncomponents);
            goto found;
        }

        if (!(searched->mode & S_IFDIR)) {
            debug("component: %lu is not directory at: %s", curdir->address,
                  path->components[n]);
            goto not_found;
        }

        n++;

        curdir = searched;

        debug("looking for: %s in %lu", path->components[n], curdir->address);
    }

found:

    if (curdir && curdir != &root) {
        ex_inode_free(curdir);
    }

not_found:
    return searched;
}

struct ex_inode *ex_inode_get(struct ex_inode *dir, const char *name) {

    struct ex_inode inode;
    ex_status status = OK;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            if (entry.free || entry.magic != EX_DIR_MAGIC1) {
                continue;
            }

            if (strcmp(entry.name, name)) {
                continue;
            }

            if ((status = ex_inode_load(entry.address, &inode)) != OK) {
                error("unable to load inode at: %lu", entry.address);
            }

            goto finded;
        }
    }

    status = INODE_NOT_FOUND;
    debug("inode=%ld does not contain=%s", dir->address, name);

finded:
    foreach_inode_block_cleanup(dir, block);

    if (status == OK) {
        return ex_copy_inode(&inode);
    } else {
        return NULL;
    }
}

int ex_dir_is_empty(struct ex_inode *inode) {

    struct ex_dir_entry **entries = ex_inode_get_all(inode);
    size_t entries_count = 0;

    for (; entries[entries_count] && entries_count <= 3; entries_count++)
        ;

    for (size_t i = 0; entries[i]; i++) {
        free(entries[i]);
    }

    free(entries);

    if (entries_count < 3) {
        return 1;
    } else {
        return 0;
    }
}

int ex_inode_is_unlinkable(struct ex_inode *inode) {

    // not a directory, we can deallocate blocks
    if (!(inode->mode & S_IFDIR)) {
        return 1;
    }

    if (ex_dir_is_empty(inode)) {
        return 1;
    }

    return 0;
}

size_t ex_inode_find_entry_address(struct ex_inode *dir, const char *name) {

    size_t address = 0;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            debug("free=%i, magic=%i, name=%s", entry.free, entry.magic,
                  entry.name);

            if (entry.magic == EX_ENTRY_MAGIC1) {
                goto not_found;
            }

            if (strcmp(name, entry.name)) {
                continue;
            }

            debug("finded: block=%ld, i=%ld", block_iterator.block_number,
                  entry_iterator.entry_number);

            address = block.address + (entry_iterator.entry_number *
                                       sizeof(struct ex_dir_entry));

            goto found;
        }
    }

not_found:


found:
    foreach_inode_block_cleanup(dir, block);

    return address;
}

void ex_dir_entry_flush(size_t address, struct ex_dir_entry *entry) {
    ex_device_write(address, (void *)entry, sizeof(struct ex_dir_entry));
}

struct ex_dir_entry *ex_dir_entry_load(size_t address) {
    struct ex_dir_entry *entry = NULL;
    // XXX: ignore status for now
    (void)ex_device_read((void **)&entry, address, sizeof(struct ex_dir_entry));
    return entry;
}

void ex_dir_entry_free(struct ex_dir_entry *entry) { free(entry); }

ex_status ex_inode_unlink(struct ex_inode *dir, const char *name) {

    debug("trying to unlink %s from %zu", name, dir->address);

    // XXX: do some checks
    size_t entry_address = ex_inode_find_entry_address(dir, name);
    struct ex_dir_entry *entry = ex_dir_entry_load(entry_address);

    struct ex_inode inode;
    ex_status status = OK;

    if ((status = ex_inode_load(entry->address, &inode)) != OK) {
        error("unable to load inode from: %lu", entry->address);
        goto done;
    }

    if (!ex_inode_is_unlinkable(&inode)) {
        debug("inode (%lu) is not unlinkable", inode.address);
        status = INODE_IS_NOT_UNLINKABLE;
        goto done;
    }

    if (inode.mode & S_IFDIR) {
        inode.nlinks -= 2;
    } else {
        inode.nlinks -= 1;
    }

    if (!inode.nlinks) {
        debug("# of inode links reached 0, deallocating blocks");
        ex_inode_deallocate_blocks(&inode);
    }

    entry->free = 1;

    ex_inode_flush(&inode);
    ex_dir_entry_flush(entry_address, entry);

done:
    free(entry);
    return status;
}

struct ex_dir_entry *ex_dir_entry_copy(const struct ex_dir_entry *entry) {

    struct ex_dir_entry *new_entry = ex_malloc(sizeof(struct ex_dir_entry));

    new_entry->address = entry->address;
    new_entry->magic = entry->magic;
    new_entry->free = entry->free;
    strcpy(new_entry->name, entry->name);

    return new_entry;
}

struct ex_dir_entry **ex_inode_get_all(struct ex_inode *dir) {

    if (!(dir->mode & S_IFDIR)) {
        warning("inode on %lu is not directory", dir->address);
        return NULL;
    }

    size_t max_direntries = 16, dir_entry_no = 0;
    struct ex_dir_entry **entries =
        ex_malloc(sizeof(struct ex_dir_entry *) * max_direntries);

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            if (entry.free) {
                continue;
            }

            // no more entries are present in current block
            if (entry.magic != EX_DIR_MAGIC1) {
                break;
            }

            entries[dir_entry_no++] = ex_dir_entry_copy(&entry);

            if (dir_entry_no >= max_direntries) {
                max_direntries <<= 1;
                entries = ex_realloc(entries, sizeof(struct ex_dir_entry *) *
                                                  max_direntries);
            }

            entries[dir_entry_no] = NULL;
        }
    }

    foreach_inode_block_cleanup(dir, block);

    return entries;
}

ssize_t ex_inode_write(struct ex_inode *ino, size_t off, const char *data,
                       size_t amount) {

    info("off=%lu, amount=%lu", off, amount);

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    if (start_block_idx >= ex_inode_max_blocks() ||
        (start_block_off + amount / EX_BLOCK_SIZE) >= ex_inode_max_blocks()) {
        return -1;
    }

    if (off + amount > ino->size) {
        ino->size += (off + amount) - ino->size;
    }

    block_address addr = ino->blocks[start_block_idx].address;
    char allocated = ino->blocks[start_block_idx].allocated;

    if (!allocated) {
        // XXX: do allocation
    }

    ex_device_write(ino->address, (void *)ino, sizeof(struct ex_inode));
    ex_device_write(addr + start_block_off, data, amount);

    return (ssize_t)amount;
}

ssize_t ex_inode_read(struct ex_inode *ino, size_t off, char *buffer,
                      size_t amount) {

    size_t start_block_idx = off / EX_BLOCK_SIZE;
    size_t start_block_off = off % EX_BLOCK_SIZE;

    if (start_block_idx >= ex_inode_max_blocks()) {
        return 0;
    }

    size_t offset = ino->blocks[start_block_idx].address + start_block_off;
    ssize_t readed = 0;
    // XXX: check if block is allocated
    // XXX: ignore status for now
    (void)ex_device_read_to_buffer(&readed, buffer, offset, amount);

    return readed;
}

int ex_inode_rename(struct ex_inode *from_inode, struct ex_inode *to_inode,
                    const char *from_name, const char *to_name) {

    if (!S_ISDIR(from_inode->mode) || !S_ISDIR(to_inode->mode)) {
        return -ENOTDIR;
    }

    size_t from_entry_address =
        ex_inode_find_entry_address(from_inode, from_name);

    if (!from_entry_address) {
        debug("directory at (%lu) does not contain: %s", from_inode->address,
              from_name);
        return -ENOENT;
    }

    size_t to_entry_address = ex_inode_find_entry_address(to_inode, to_name);

    if (!to_entry_address) {
        debug("directory at (%lu) does not contain: %s, allocating new space",
              from_inode->address, from_name);

        to_entry_address = ex_inode_find_free_entry_address(to_inode);

        // XXX: use function for this, change return values
        if (!to_entry_address) {

            info("unable to find space for inode in: %zu", to_inode->number);

            if (ex_inode_allocate_indirect_block(to_inode, to_inode->nblocks) != OK) {
                return -ENOSPC;
            }

            to_entry_address = ex_inode_find_free_entry_address(to_inode);

            if (!to_entry_address) {
                warning("unable to find free space after relocation");
            }

            return -ENOSPC;
        }

    }

    debug("from/to: %lu/%lu", from_entry_address, to_entry_address);

    struct ex_dir_entry *from_entry = ex_dir_entry_load(from_entry_address);

    ex_inode_entry_update(to_entry_address, to_name, from_entry->address, 0);

    from_entry->free = 1;
    ex_dir_entry_flush(from_entry_address, from_entry);

    return 0;
}

static inline int ex_indirect_block_is_allocated(struct ex_inode *i, size_t n) {
    return i->blocks[n + EX_DIRECT_BLOCKS].address;
}

static inline int ex_indirect_block_out_of_range(size_t n) {
    return n > EX_INDIRECT_BLOCKS;
}

static inline int ex_indirect_block_should_be_loaded(struct ex_block_iterator *it,
                                             size_t indirect_block_no) {
    return it->indirect_block_no != indirect_block_no || !it->indirect.data;
}

struct ex_inode_block ex_inode_block_iterate_indirect(struct ex_inode *inode,
                                                      struct ex_block_iterator *it) {
    assert(it->block_number >= EX_DIRECT_BLOCKS);

    size_t __ind = (it->block_number - EX_DIRECT_BLOCKS);

    size_t indirect_block_no = __ind / blocks_per_block;
    size_t block_idx = __ind % blocks_per_block;

    debug("ibno: %zu, bi: %zu", indirect_block_no, block_idx);

    if (ex_indirect_block_out_of_range(indirect_block_no) ||
        !ex_indirect_block_is_allocated(inode, indirect_block_no)) {

        debug("not allocated or out of range");

        it->last_block.address = EX_BLOCK_INVALID_ADDRESS;
        it->last_block.data = NULL;
        it->last_block.id = EX_BLOCK_INVALID_ID;

        goto done;
    }

    if (ex_indirect_block_should_be_loaded(it, indirect_block_no)) {
        size_t indirect_block_address = inode->blocks[indirect_block_no + EX_DIRECT_BLOCKS].address;

        debug("loading indirect block: %zu at %zu", indirect_block_no, indirect_block_address);
        ssize_t readed = 0;
        (void)ex_device_read_to_buffer(&readed, it->indirect_buffer,
                                       indirect_block_address, EX_BLOCK_SIZE);
        it->indirect.data = it->indirect_buffer;
    }

    // XXX: this is the end, my only friend, the end
    struct ex_data_block block = ((struct ex_data_block *)it->indirect.data)[block_idx];

    debug("ba: %zu, aloc: %zu", block.address, block.allocated);

    // block in the indirect block is not allocated
    if (!block.allocated) {
        it->last_block.address = EX_BLOCK_INVALID_ADDRESS;
        it->last_block.data = NULL;
        it->last_block.id = EX_BLOCK_INVALID_ID;

        goto done;
    }

    // read the blocks data
    ssize_t readed = 0;
    (void)ex_device_read_to_buffer(&readed, it->buffer, block.address, EX_BLOCK_SIZE);

    it->last_block.address = block.address;
    it->last_block.id = it->block_number;
    it->last_block.data = it->buffer;

done:
    return it->last_block;
}

struct ex_inode_block ex_inode_block_iterate(struct ex_inode *inode,
                                             struct ex_block_iterator *it) {

    // not first iteration
    if (it->last_block.data) {
        it->block_number++;
    }

    struct ex_inode_block block = {.data = NULL,
                                   .id = EX_BLOCK_INVALID_ID,
                                   .address = EX_BLOCK_INVALID_ADDRESS};

    if (it->block_number >= EX_DIRECT_BLOCKS) {
        debug("iterating block: %zu", it->block_number);
        return ex_inode_block_iterate_indirect(inode, it);
    }

    block.address = inode->blocks[it->block_number].address;
    block.data = it->buffer;
    block.id = it->block_number;

    // XXX: handle read error
    ssize_t readed = 0;
    (void)ex_device_read_to_buffer(&readed, it->buffer,
                                   block.address, EX_BLOCK_SIZE);

    it->last_block = block;
    return block;
}

struct ex_dir_entry ex_inode_entry_iterate(struct ex_inode_block block,
                                           struct ex_entry_iterator *it) {

    // not a first iteration
    if (block.data && it->last_entry.magic) {
        it->entry_number++;
    }

    if (it->entry_number >= EX_BLOCK_SIZE / sizeof(struct ex_dir_entry)) {
        goto end;
    }

    size_t entry_offset = it->entry_number * sizeof(struct ex_dir_entry);
    char *entry_data = &block.data[entry_offset];

    it->last_entry = *((struct ex_dir_entry *)entry_data);

    return it->last_entry;

end:
    it->last_entry.magic = 0;

    return it->last_entry;
}

size_t ex_inode_max_blocks(void) { return EX_DIRECT_BLOCKS + EX_INDIRECT_BLOCKS * blocks_per_block; }

int ex_inode_has_perm(struct ex_inode *ino, ex_permission perm, gid_t gid, uid_t uid) {

    char other = ino->mode & 7;
    char group = (ino->mode >> 3) & 7;
    char user = (ino->mode >> 6) & 7;

    char user_is_owner = ino->uid == uid;
    char group_is_owner = ino->gid == gid;

    if (user_is_owner && group_is_owner) {
        return (group & perm) || (user & perm);
    } else if (user_is_owner) {
        return user & perm;
    } else if (group_is_owner) {
        return group & perm;
    } else {
        return other & perm;
    }

    return 0;
}

