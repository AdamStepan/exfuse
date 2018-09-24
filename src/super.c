#include <super.h>

struct ex_super_block *super_block = NULL;

void ex_super_deallocate_block(block_address address) {

    // compute position of block in bitmap
    block_address first = super_block->bitmap + super_block->bitmap_size;
    block_address without_offset = address - first;

    // now, block must be divisible by EX_BLOCK_SIZE
    size_t nth_bit = without_offset / EX_BLOCK_SIZE;

    size_t nth_byte = nth_bit / 8;
    size_t nth_bit_in_byte = nth_bit % 8;

    info("freeing block address=%lu, nthbyte=%lu, nthbit=%lu",
         address, nth_byte, nth_bit_in_byte);

    // set nthbit to false
    char *bitmap = ex_device_read(super_block->bitmap + nth_byte, sizeof(char));

    *bitmap &= ~(1UL << nth_bit_in_byte);

    ex_device_write(super_block->bitmap + nth_byte, bitmap, sizeof(char));

    free(bitmap);
}


block_address ex_super_allocate_block(void) {

    char *bitmap = ex_device_read(super_block->bitmap,
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

    warning("unable to find free block");

    free(bitmap);
    return -1;
finded:
    debug("found free block: position=%lu", free_block_pos);

    // XXX: we flip only one bit, we don't need to the rewrite whole bitmap
    ex_device_write(super_block->bitmap, bitmap, super_block->bitmap_size);
    free(bitmap);

    // compute block position
    return (super_block->bitmap + super_block->bitmap_size) +
        free_block_pos * EX_BLOCK_SIZE;
}

void ex_super_print(const struct ex_super_block *block) {
    info("{.root=%lu, .device_size=%lu, bitmap=%lu, bitmap_size=%lu",
            block->root, block->device_size, block->bitmap, block->bitmap_size);
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

    *super_block = (struct ex_super_block){
        .root = root_addr,
        .device_size = device_size,
        .bitmap = bitmap_addr,
        .bitmap_size = bitmap_size
    };

    int fd = ex_device_fd();
    write(fd, super_block, sizeof(struct ex_super_block));
}

void ex_super_load(void) {

    info("loading device");

    super_block = ex_device_read(0, sizeof(struct ex_super_block));
    ex_super_print(super_block);
}
