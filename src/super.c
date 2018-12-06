#include <super.h>

struct ex_super_block *super_block = NULL;

// super | inode bitmap | data bitmap | inodes | data
#define inode_bitmap_end \
    (super_block->inode_bitmap.address + (super_block->inode_bitmap.size / 8))

#define data_bitmap_end \
    (super_block->bitmap.address + (super_block->bitmap.size / 8))

#define first_data_block \
    (first_inode_block + (EX_BLOCK_SIZE * super_block->inode_bitmap.size))

#define first_inode_block (data_bitmap_end)

void ex_bitmap_free_bit(struct ex_bitmap *bitmap, size_t nth_bit) {

    size_t nth_byte = nth_bit / 8;
    size_t nth_bit_in_byte = nth_bit % 8;

    char *bitdata = ex_device_read(bitmap->address + nth_byte, sizeof(char));

    *bitdata &= ~(1UL << nth_bit_in_byte);

    if(bitmap->allocated)
        bitmap->allocated -= 1;

    ex_device_write(bitmap->address + nth_byte, bitdata, sizeof(char));
    ex_device_write(bitmap->head, (void *)bitmap, sizeof(struct ex_bitmap));

    free(bitdata);
}

size_t ex_bitmap_find_free_bit(struct ex_bitmap *bitmap) {

    if(bitmap->allocated == bitmap->size) {
        info("bitmap is full");
        return -1;
    }

    char *bitdata = ex_device_read(bitmap->address, bitmap->size);
    size_t bitpos = -1, bytepos;

    // go through all bits in bitmap, find first free bit
    for(bytepos = 0; bytepos < bitmap->size; bytepos++) {
        for(uint8_t bit = 0; bit < 8; bit++) {
            // n = (8 * i) + bit, nth block is occupied
            if(bitdata[bytepos] & (1 << bit)) {
                continue;
            }

            // flip bit, compute absolute btt position
            bitdata[bytepos] |= (1 << bit);
            bitpos = (8 * bytepos) + bit;

            bitmap->allocated += 1;

            goto found;
        }
    }

found:
    ex_device_write(bitmap->address + bytepos, bitdata + bytepos, sizeof(char));
    ex_device_write(bitmap->head, (void *)bitmap, sizeof(struct ex_bitmap));

    free(bitdata);

    return bitpos;
}

void ex_super_deallocate_block(block_address address) {

    // compute position of block in bitmap
    size_t address_without_offset = address - first_data_block;

    // now, block must be divisible by EX_BLOCK_SIZE
    size_t nth_bit = address_without_offset / EX_BLOCK_SIZE;

    ex_bitmap_free_bit(&super_block->bitmap, nth_bit);

    debug("nth=%lu", nth_bit);

}

void ex_super_init_block(size_t address, char with) {

    static char FREE_BLOCK[EX_BLOCK_SIZE];
    memset(FREE_BLOCK, with, EX_BLOCK_SIZE);

    ex_device_write(address, FREE_BLOCK, sizeof(FREE_BLOCK));
}

struct ex_inode_block ex_super_allocate_inode_block(void) {

    struct ex_inode_block block = {
        .address = EX_BLOCK_INVALID_ADDRESS,
        .id = EX_BLOCK_INVALID_ID
    };

    block.id = ex_bitmap_find_free_bit(&super_block->inode_bitmap);

    if(block.id == EX_BLOCK_INVALID_ID) {
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

    if(free_block_pos == EX_BLOCK_INVALID_ID) {
        warning("unable to find free block");
    } else {
        debug("found free block: position=%lu", free_block_pos);

        address = first_data_block + (free_block_pos * EX_BLOCK_SIZE);
        ex_super_init_block(address, 'a');
    }

    return address;
}

void ex_super_print(const struct ex_super_block *block) {
    info("{.root=%lu, .device_size=%lu, .bitmap={head=%lu, .address=%lu .size=%lu, .allocated=%lu}}",
            block->root, block->device_size, block->bitmap.head, block->bitmap.address,
            block->bitmap.size, block->bitmap.allocated);
}

void ex_super_statfs(struct statvfs *statbuf) {

    statbuf->f_bsize = EX_BLOCK_SIZE;
    statbuf->f_namemax = EX_NAME_LEN;

    statbuf->f_blocks = super_block->bitmap.size;
    statbuf->f_bfree = super_block->bitmap.size - super_block->bitmap.allocated;
    statbuf->f_bavail = statbuf->f_bfree;

    statbuf->f_files = super_block->inode_bitmap.size;
    statbuf->f_ffree = statbuf->f_files - super_block->inode_bitmap.allocated;
}

void ex_super_write(size_t device_size) {

#define EX_MAX_INODES 128

    size_t size = device_size - sizeof(struct ex_super_block);
    info("populating device, size=%lu", size);

    size_t inode_bitmap_size = EX_MAX_INODES;
    size_t data_bitmap_size = ((size / EX_BLOCK_SIZE) - EX_MAX_INODES) * 8;
    info("inode_bitmap_size=%lu, data_bitmap_size=%lu", inode_bitmap_size,
            data_bitmap_size);

    struct ex_bitmap inode_bitmap = {
        .head = offsetof(struct ex_super_block, inode_bitmap),
        .address = sizeof(struct ex_super_block),
        .allocated = 0,
        .size = inode_bitmap_size
    };

    struct ex_bitmap bitmap = {
        .head = offsetof(struct ex_super_block, bitmap),
        .address = inode_bitmap.address + inode_bitmap.size / 8,
        .allocated = 0,
        .size = data_bitmap_size
    };

    super_block = ex_malloc(sizeof(struct ex_super_block));

    *super_block = (struct ex_super_block){
        .root = 0,
        .device_size = device_size,
        .bitmap = bitmap,
        .inode_bitmap = inode_bitmap,
        .magic = EX_SUPER_MAGIC
    };

    ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));
}

pthread_mutex_t super_lock;

void ex_super_load(void) {

    info("loading device");

    super_block = ex_device_read(0, sizeof(struct ex_super_block));

    if(!super_block) {
        fatal("unable to load super block");
    }

    if(super_block->magic != EX_SUPER_MAGIC) {
        fatal("invalid super block magic: %x, expected: %x",
                super_block->magic, EX_SUPER_MAGIC);
    }


    pthread_mutex_init(&super_lock, NULL);
}

int ex_super_check_path_len(const char *pathname) {
    return strlen(pathname) <= EX_NAME_LEN;
}

void ex_super_lock(void) {
    pthread_mutex_lock(&super_lock);
}

void ex_super_unlock(void) {
    pthread_mutex_unlock(&super_lock);
}
