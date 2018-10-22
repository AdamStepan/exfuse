#include <super.h>

struct ex_super_block *super_block = NULL;

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
    size_t first_data_block = super_block->bitmap.address + super_block->bitmap.size;
    size_t address_without_offset = address - first_data_block;

    // now, block must be divisible by EX_BLOCK_SIZE
    size_t nth_bit = address_without_offset / EX_BLOCK_SIZE;

    ex_bitmap_free_bit(&super_block->bitmap, nth_bit);

    debug("nth=%lu", nth_bit);

}

block_address ex_super_allocate_block(void) {

    size_t free_block_pos = ex_bitmap_find_free_bit(&super_block->bitmap);
    size_t address = -1;


    if(free_block_pos == -1) {
        warning("unable to find free block");
    } else {
        debug("found free block: position=%lu", free_block_pos);

        address = (super_block->bitmap.address + super_block->bitmap.size) +
            free_block_pos * EX_BLOCK_SIZE;
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
    statbuf->f_blocks = super_block->bitmap.size;
    statbuf->f_bfree = super_block->bitmap.size - super_block->bitmap.allocated;
    statbuf->f_bavail = statbuf->f_bfree;

    statbuf->f_namemax = EX_NAME_LEN;

    // XXX: add f_files and f_ffree
}

void ex_super_write(size_t device_size) {

    info("populating device: device_size=%lu, blocks=%lu", device_size, device_size / EX_BLOCK_SIZE);
    info("super_block: block end=%lu", sizeof(struct ex_super_block));

    // every byte in bitmap represent 8 pages
    size_t bitmap_size = device_size / (EX_BLOCK_SIZE * 8);

    info("super_block: bitmap device_size=%lu", bitmap_size);

    size_t bitmap_addr = sizeof(struct ex_super_block);
    size_t root_addr = bitmap_addr + bitmap_size;

    info("super_block: bitmap addr=%lu", bitmap_addr);
    info("super_block: root_addr=%lu", root_addr);

    super_block = ex_malloc(sizeof(struct ex_super_block));

    struct ex_bitmap bitmap = {
        .head = offsetof(struct ex_super_block, bitmap),
        .address = sizeof(struct ex_super_block),
        .allocated = 0,
        .size = device_size / (EX_BLOCK_SIZE * 8)
    };

    *super_block = (struct ex_super_block){
        .root = root_addr,
        .device_size = device_size,
        .bitmap = bitmap,
        .magic = EX_SUPER_MAGIC
    };

    ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));
}

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
}

int ex_super_check_path_len(const char *pathname) {
    return strlen(pathname) <= EX_NAME_LEN;
}
