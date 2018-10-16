#include <super.h>

struct ex_super_block *super_block = NULL;

void ex_super_deallocate_block(block_address address) {

    // compute position of block in bitmap
    size_t first_data_block = super_block->bitmap + super_block->bitmap_size - 8;
    size_t address_without_offset = address - first_data_block;

    // now, block must be divisible by EX_BLOCK_SIZE, -1 because first block is on
    // address 0
    size_t nth_bit = (address_without_offset / EX_BLOCK_SIZE) - 1;

    size_t nth_byte = nth_bit / 8;
    size_t nth_bit_in_byte = nth_bit % 8;

    info("freeing block address=%lu, nthbyte=%lu, nthbit=%lu, pos=%lu",
         address, nth_byte, nth_bit_in_byte, nth_bit);

    // set nthbit to false
    char *bitmap = ex_device_read(super_block->bitmap + nth_byte, sizeof(char));

    *bitmap &= ~(1UL << nth_bit_in_byte);

    super_block->blocks_free += 1;

    ex_device_write(super_block->bitmap + nth_byte, bitmap, sizeof(char));
    ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));

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
            info("allocating=%lu, %i, pos=%lu", i, bit, free_block_pos);
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

    if(super_block->blocks_free)
        super_block->blocks_free -= 1;

    ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));

    // compute block position
    return (super_block->bitmap + super_block->bitmap_size) +
        free_block_pos * EX_BLOCK_SIZE;
}

void ex_super_print(const struct ex_super_block *block) {
    info("{.root=%lu, .device_size=%lu, .bitmap=%lu, .bitmap_size=%lu, .free=%lu}",
            block->root, block->device_size, block->bitmap, block->bitmap_size,
            block->blocks_free);
}

void ex_super_statfs(struct statvfs *statbuf) {

    statbuf->f_bsize = EX_BLOCK_SIZE;
    statbuf->f_blocks = super_block->bitmap_size;
    statbuf->f_bfree = statbuf->f_bavail = super_block->blocks_free;
    statbuf->f_namemax = EX_NAME_LEN;

    ex_super_print(super_block);

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

    *super_block = (struct ex_super_block){
        .root = root_addr,
        .device_size = device_size,
        .bitmap = bitmap_addr,
        .bitmap_size = bitmap_size,
        .blocks_free = bitmap_size
    };

    ex_device_write(0, (char *)super_block, sizeof(struct ex_super_block));
}

void ex_super_load(void) {

    info("loading device");

    super_block = ex_device_read(0, sizeof(struct ex_super_block));
    ex_super_print(super_block);
}

int ex_super_check_path_len(const char *pathname) {
    return strlen(pathname) <= EX_NAME_LEN;
}
