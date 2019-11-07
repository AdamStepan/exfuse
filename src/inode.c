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

    return ex_device_write(0, (char *)super_block,
                           sizeof(struct ex_super_block));
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

ex_status ex_inode_allocate_blocks(struct ex_inode *inode) {

    ex_status status = OK;

    debug("allocating blocks for inode (%lu)", inode->number);

    for (size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {

        struct ex_inode_block block;

        if ((status = ex_super_allocate_data_block(&block)) != OK) {
            warning("failing to allocate nth (%lu) block", i);
            goto done;
        }

        inode->blocks[i] = block.address;
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

        if (inode->blocks[i] == EX_BLOCK_INVALID_ADDRESS) {
            break;
        }

        ex_super_deallocate_block(inode->blocks[i]);
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

    info("uid: %u", inode->uid);
    info("gid: %u", inode->gid);

    info("mtime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    info("atime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
    info("ctime: %ld.%.9ld", inode->mtime.tv_sec, inode->mtime.tv_nsec);
}

ex_status ex_inode_flush(const struct ex_inode *inode) {

    ex_status status =
        ex_device_write(inode->address, (void *)inode, sizeof(struct ex_inode));

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

    for (size_t i = 0; i < EX_DIRECT_BLOCKS; i++) {
        copy->blocks[i] = inode->blocks[i];
    }

    return copy;
}

void ex_inode_fill_dir(struct ex_inode *inode, struct ex_inode *parent) {

    ex_inode_set(inode, ".", inode);
    ex_inode_set(inode, "..", parent);

    ex_inode_flush(inode);
}

ex_status ex_inode_create(struct ex_inode *inode, uint16_t mode, gid_t gid,
                          uid_t uid) {

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

    size_t address = 0;

    foreach_inode_block(dir, block) {
        foreach_block_entry(block, entry) {

            debug("free=%i, magic=%x, blockaddr=%lu", entry.free, entry.magic,
                  block.address);

            if (!entry.free && entry.magic != EX_ENTRY_MAGIC1) {
                continue;
            }

            debug("finded: block=%ld, i=%ld", block_iterator.block_number,
                  entry_iterator.entry_number);

            address = block.address + (entry_iterator.entry_number *
                                       sizeof(struct ex_dir_entry));

            goto found;
        }
    }

found:
    foreach_inode_block_cleanup(dir, block);

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

struct ex_inode *ex_inode_set(struct ex_inode *dir, const char *name,
                              struct ex_inode *inode) {

    // XXX: we should check if `name' is not present in a `dir`
    size_t entry_address = ex_inode_find_free_entry_address(dir);

    if (!entry_address) {
        info("unable to find space for inode in dirinode");
        return NULL;
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

    if (ex_path_is_root(path)) {
        return ex_copy_inode(&root);
    }

    struct ex_inode *current = NULL, *current_dir = &root;

    for (size_t n = 0; n < path->ncomponents; n++) {

        int is_last_component = n + 1 == path->ncomponents;
        int is_dir = current_dir->mode & S_IFDIR;

        if (!is_last_component && !is_dir) {
            debug("%zu is not a directory", current_dir->number);
        }

        debug("looking for %s in %zu", path->components[n], current_dir->number);
        current = ex_inode_get(current_dir, path->components[n]);

        if (!current)
            goto done;

        if (current->mode & S_IFDIR) {

            if (current_dir != &root) {
                ex_inode_free(current_dir);
            }

            debug("set current dir to %zu", current->number);
            current_dir = current;
        }

    }

done:

    if (current_dir != &root && current_dir != current) {
        ex_inode_free(current_dir);
    }

    return current;
}

struct ex_inode *ex_inode_get(struct ex_inode *dir, const char *name) {

    if (!(dir->mode & S_IFDIR)) {
        debug("dir is not a directory: %zu", dir->number);
        return NULL;
    }

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

    block_address addr = ino->blocks[start_block_idx];

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

    size_t offset = ino->blocks[start_block_idx] + start_block_off;
    ssize_t readed = 0;

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

        if (!to_entry_address) {
            debug("unable to find a free entry address, inode: %ld",
                  to_inode->number);
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
        goto done;
    }

    block.address = inode->blocks[it->block_number];
    block.data = it->buffer;

    // XXX: add buffer into ex_block_iterator and use ex_device_read_to_buffer
    //      handle read error
    ssize_t readed = 0;
    // XXX: handle read error
    (void)ex_device_read_to_buffer(&readed, it->buffer, block.address,
                                   EX_BLOCK_SIZE);

done:
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

size_t ex_inode_max_blocks(void) { return EX_DIRECT_BLOCKS; }

int ex_inode_has_perm(struct ex_inode *ino, ex_permission perm, gid_t gid,
                      uid_t uid) {

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
