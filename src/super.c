#include "super.h"

struct ex_super_block *super_block = NULL;

// super | inode bitmap | data bitmap | inodes | data
#define inode_bitmap_end                                                       \
    (super_block->inode_bitmap.address + super_block->inode_bitmap.size)

#define data_bitmap_end (super_block->bitmap.address + super_block->bitmap.size)

#define first_data_block                                                       \
    (first_inode_block + (EX_BLOCK_SIZE * super_block->inode_bitmap.max_items))

#define first_inode_block (data_bitmap_end)

void ex_bitmap_free_bit(struct ex_bitmap *bitmap, size_t nth_bit) {

    size_t nth_byte = nth_bit / 8;
    size_t nth_bit_in_byte = nth_bit % 8;

    char *bitdata = NULL;

    // XXX: ignore status for now
    (void)ex_device_read((void **)&bitdata, bitmap->address + nth_byte, sizeof(char));

    *bitdata &= ~(1UL << nth_bit_in_byte);

    if (bitmap->allocated)
        bitmap->allocated -= 1;

    ex_device_write(bitmap->address + nth_byte, bitdata, sizeof(char));
    ex_device_write(bitmap->head, (void *)bitmap, sizeof(struct ex_bitmap));

    free(bitdata);
}

size_t ex_bitmap_find_free_bit(struct ex_bitmap *bitmap) {

    if (bitmap->allocated == bitmap->max_items) {
        info("bitmap is full");
        return -1;
    }

    char *bitdata = NULL;
    // XXX: ignore status for now
    (void)ex_device_read((void **)&bitdata, bitmap->address, bitmap->size);
    size_t bitpos = -1, bytepos = bitmap->last, startpos = bitmap->last;
    char flipped = 0;

    // go through all bits in bitmap, find first free bit
    while (1) {

        // XXX: check bitmap->last < bitmap->size

        for (uint8_t bit = 0; bit < 8; bit++) {

            // n = (8 * i) + bit, nth block is occupied
            if (bitdata[bytepos] & (1 << bit)) {
                continue;
            }

            // flip bit, compute absolute bit position
            bitdata[bytepos] |= (1 << bit);
            bitpos = (8 * bytepos) + bit;

            bitmap->allocated += 1;
            bitmap->last = bytepos;

            goto found;
        }

        bytepos++;

        // we went through the whole bitmap
        if (flipped && startpos == bytepos) {
            bitpos = -1;
            goto not_found;
        }

        if (bytepos >= bitmap->size) {
            flipped = 1;
            bytepos = 0;
        }
    }

found:
    ex_device_write(bitmap->address + bytepos, bitdata + bytepos, sizeof(char));
    ex_device_write(bitmap->head, (void *)bitmap, sizeof(struct ex_bitmap));

not_found:
    free(bitdata);
    return bitpos;
}

void ex_super_deallocate_block(block_address address) {

    // compute position of block in bitmap
    size_t address_without_offset = address - first_data_block;

    // now, block must be divisible by EX_BLOCK_SIZE
    size_t nth_bit = address_without_offset / EX_BLOCK_SIZE;

    ex_bitmap_free_bit(&super_block->bitmap, nth_bit);
}

void ex_super_init_block(size_t address, char with) {

    static char FREE_BLOCK[EX_BLOCK_SIZE];
    memset(FREE_BLOCK, with, EX_BLOCK_SIZE);

    ex_device_write(address, FREE_BLOCK, sizeof(FREE_BLOCK));
}

struct ex_inode_block ex_super_allocate_inode_block(void) {

    struct ex_inode_block block = {.address = EX_BLOCK_INVALID_ADDRESS,
                                   .id = EX_BLOCK_INVALID_ID};

    block.id = ex_bitmap_find_free_bit(&super_block->inode_bitmap);

    if (block.id == EX_BLOCK_INVALID_ID) {
        warning("unable to find free inode block");
        goto returnblock;
    }

    block.address = first_inode_block + block.id * EX_BLOCK_SIZE;

    ex_super_init_block(block.address, 0);
returnblock:
    return block;
}

void ex_super_deallocate_inode_block(size_t inode_number) {
    ex_bitmap_free_bit(&super_block->inode_bitmap, inode_number);
}

block_address ex_super_allocate_block(void) {

    size_t free_block_pos = ex_bitmap_find_free_bit(&super_block->bitmap);
    size_t address = -1;

    if (free_block_pos == EX_BLOCK_INVALID_ID) {
        warning("unable to find free block");
    } else {
        debug("found free block: position=%lu", free_block_pos);

        address = first_data_block + (free_block_pos * EX_BLOCK_SIZE);
        ex_super_init_block(address, 'a');
    }

    return address;
}

void ex_super_print(const struct ex_super_block *block) {
    debug("{.root=%lu, .device_size=%lu, .bitmap={head=%lu, .address=%lu "
          ".size=%lu, .allocated=%lu}}",
          block->root, block->device_size, block->bitmap.head,
          block->bitmap.address, block->bitmap.size, block->bitmap.allocated);
}

void ex_super_statfs(struct statvfs *statbuf) {

    statbuf->f_bsize = EX_BLOCK_SIZE;
    statbuf->f_namemax = EX_NAME_LEN;

    statbuf->f_blocks = super_block->bitmap.max_items;
    statbuf->f_bfree =
        super_block->bitmap.max_items - super_block->bitmap.allocated;
    statbuf->f_bavail = statbuf->f_bfree;

    statbuf->f_files = super_block->inode_bitmap.max_items;
    statbuf->f_ffree = statbuf->f_files - super_block->inode_bitmap.allocated;

    statbuf->f_flag = ST_SYNCHRONOUS | ST_NOSUID | ST_NODEV;
}

pthread_mutex_t super_lock;
pthread_mutexattr_t super_lock_attr;

void ex_super_load(void) {

    info("loading device");

    // XXX: ignore status for now
    (void)ex_device_read((void **)&super_block, 0, sizeof(struct ex_super_block));

    if (!super_block) {
        fatal("unable to load super block");
    }

    if (super_block->magic != EX_SUPER_MAGIC) {
        fatal("invalid super block magic: %x, expected: %x", super_block->magic,
              EX_SUPER_MAGIC);
    }

    pthread_mutexattr_init(&super_lock_attr);
    pthread_mutexattr_settype(&super_lock_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&super_lock, &super_lock_attr);
}

int ex_super_check_path_len(const char *pathname) {

    struct ex_path *path = ex_path_make(pathname);

    for (size_t i = 0; i < path->ncomponents; i++) {
        if (strlen(path->components[i]) >= EX_NAME_LEN)
            return 0;
    }

    return 1;
}

void ex_super_lock(void) { pthread_mutex_lock(&super_lock); }

void ex_super_unlock(void) { pthread_mutex_unlock(&super_lock); }
