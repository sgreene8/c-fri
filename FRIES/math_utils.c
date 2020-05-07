/*! \file
 *
 * \brief Miscellaneous math utilities and definitions
 */

#include "math_utils.h"

/*const uint8_t byte_pos[256][8] = {
    {0,0,0,0,0,0,0,0,},
    {0,0,0,0,0,0,0,0,},
    {1,0,0,0,0,0,0,0,},
    {0,1,0,0,0,0,0,0,},
    {2,0,0,0,0,0,0,0,},
    {0,2,0,0,0,0,0,0,},
    {1,2,0,0,0,0,0,0,},
    {0,1,2,0,0,0,0,0,},
    {3,0,0,0,0,0,0,0,},
    {0,3,0,0,0,0,0,0,},
    {1,3,0,0,0,0,0,0,},
    {0,1,3,0,0,0,0,0,},
    {2,3,0,0,0,0,0,0,},
    {0,2,3,0,0,0,0,0,},
    {1,2,3,0,0,0,0,0,},
    {0,1,2,3,0,0,0,0,},
    {4,0,0,0,0,0,0,0,},
    {0,4,0,0,0,0,0,0,},
    {1,4,0,0,0,0,0,0,},
    {0,1,4,0,0,0,0,0,},
    {2,4,0,0,0,0,0,0,},
    {0,2,4,0,0,0,0,0,},
    {1,2,4,0,0,0,0,0,},
    {0,1,2,4,0,0,0,0,},
    {3,4,0,0,0,0,0,0,},
    {0,3,4,0,0,0,0,0,},
    {1,3,4,0,0,0,0,0,},
    {0,1,3,4,0,0,0,0,},
    {2,3,4,0,0,0,0,0,},
    {0,2,3,4,0,0,0,0,},
    {1,2,3,4,0,0,0,0,},
    {0,1,2,3,4,0,0,0,},
    {5,0,0,0,0,0,0,0,},
    {0,5,0,0,0,0,0,0,},
    {1,5,0,0,0,0,0,0,},
    {0,1,5,0,0,0,0,0,},
    {2,5,0,0,0,0,0,0,},
    {0,2,5,0,0,0,0,0,},
    {1,2,5,0,0,0,0,0,},
    {0,1,2,5,0,0,0,0,},
    {3,5,0,0,0,0,0,0,},
    {0,3,5,0,0,0,0,0,},
    {1,3,5,0,0,0,0,0,},
    {0,1,3,5,0,0,0,0,},
    {2,3,5,0,0,0,0,0,},
    {0,2,3,5,0,0,0,0,},
    {1,2,3,5,0,0,0,0,},
    {0,1,2,3,5,0,0,0,},
    {4,5,0,0,0,0,0,0,},
    {0,4,5,0,0,0,0,0,},
    {1,4,5,0,0,0,0,0,},
    {0,1,4,5,0,0,0,0,},
    {2,4,5,0,0,0,0,0,},
    {0,2,4,5,0,0,0,0,},
    {1,2,4,5,0,0,0,0,},
    {0,1,2,4,5,0,0,0,},
    {3,4,5,0,0,0,0,0,},
    {0,3,4,5,0,0,0,0,},
    {1,3,4,5,0,0,0,0,},
    {0,1,3,4,5,0,0,0,},
    {2,3,4,5,0,0,0,0,},
    {0,2,3,4,5,0,0,0,},
    {1,2,3,4,5,0,0,0,},
    {0,1,2,3,4,5,0,0,},
    {6,0,0,0,0,0,0,0,},
    {0,6,0,0,0,0,0,0,},
    {1,6,0,0,0,0,0,0,},
    {0,1,6,0,0,0,0,0,},
    {2,6,0,0,0,0,0,0,},
    {0,2,6,0,0,0,0,0,},
    {1,2,6,0,0,0,0,0,},
    {0,1,2,6,0,0,0,0,},
    {3,6,0,0,0,0,0,0,},
    {0,3,6,0,0,0,0,0,},
    {1,3,6,0,0,0,0,0,},
    {0,1,3,6,0,0,0,0,},
    {2,3,6,0,0,0,0,0,},
    {0,2,3,6,0,0,0,0,},
    {1,2,3,6,0,0,0,0,},
    {0,1,2,3,6,0,0,0,},
    {4,6,0,0,0,0,0,0,},
    {0,4,6,0,0,0,0,0,},
    {1,4,6,0,0,0,0,0,},
    {0,1,4,6,0,0,0,0,},
    {2,4,6,0,0,0,0,0,},
    {0,2,4,6,0,0,0,0,},
    {1,2,4,6,0,0,0,0,},
    {0,1,2,4,6,0,0,0,},
    {3,4,6,0,0,0,0,0,},
    {0,3,4,6,0,0,0,0,},
    {1,3,4,6,0,0,0,0,},
    {0,1,3,4,6,0,0,0,},
    {2,3,4,6,0,0,0,0,},
    {0,2,3,4,6,0,0,0,},
    {1,2,3,4,6,0,0,0,},
    {0,1,2,3,4,6,0,0,},
    {5,6,0,0,0,0,0,0,},
    {0,5,6,0,0,0,0,0,},
    {1,5,6,0,0,0,0,0,},
    {0,1,5,6,0,0,0,0,},
    {2,5,6,0,0,0,0,0,},
    {0,2,5,6,0,0,0,0,},
    {1,2,5,6,0,0,0,0,},
    {0,1,2,5,6,0,0,0,},
    {3,5,6,0,0,0,0,0,},
    {0,3,5,6,0,0,0,0,},
    {1,3,5,6,0,0,0,0,},
    {0,1,3,5,6,0,0,0,},
    {2,3,5,6,0,0,0,0,},
    {0,2,3,5,6,0,0,0,},
    {1,2,3,5,6,0,0,0,},
    {0,1,2,3,5,6,0,0,},
    {4,5,6,0,0,0,0,0,},
    {0,4,5,6,0,0,0,0,},
    {1,4,5,6,0,0,0,0,},
    {0,1,4,5,6,0,0,0,},
    {2,4,5,6,0,0,0,0,},
    {0,2,4,5,6,0,0,0,},
    {1,2,4,5,6,0,0,0,},
    {0,1,2,4,5,6,0,0,},
    {3,4,5,6,0,0,0,0,},
    {0,3,4,5,6,0,0,0,},
    {1,3,4,5,6,0,0,0,},
    {0,1,3,4,5,6,0,0,},
    {2,3,4,5,6,0,0,0,},
    {0,2,3,4,5,6,0,0,},
    {1,2,3,4,5,6,0,0,},
    {0,1,2,3,4,5,6,0,},
    {7,0,0,0,0,0,0,0,},
    {0,7,0,0,0,0,0,0,},
    {1,7,0,0,0,0,0,0,},
    {0,1,7,0,0,0,0,0,},
    {2,7,0,0,0,0,0,0,},
    {0,2,7,0,0,0,0,0,},
    {1,2,7,0,0,0,0,0,},
    {0,1,2,7,0,0,0,0,},
    {3,7,0,0,0,0,0,0,},
    {0,3,7,0,0,0,0,0,},
    {1,3,7,0,0,0,0,0,},
    {0,1,3,7,0,0,0,0,},
    {2,3,7,0,0,0,0,0,},
    {0,2,3,7,0,0,0,0,},
    {1,2,3,7,0,0,0,0,},
    {0,1,2,3,7,0,0,0,},
    {4,7,0,0,0,0,0,0,},
    {0,4,7,0,0,0,0,0,},
    {1,4,7,0,0,0,0,0,},
    {0,1,4,7,0,0,0,0,},
    {2,4,7,0,0,0,0,0,},
    {0,2,4,7,0,0,0,0,},
    {1,2,4,7,0,0,0,0,},
    {0,1,2,4,7,0,0,0,},
    {3,4,7,0,0,0,0,0,},
    {0,3,4,7,0,0,0,0,},
    {1,3,4,7,0,0,0,0,},
    {0,1,3,4,7,0,0,0,},
    {2,3,4,7,0,0,0,0,},
    {0,2,3,4,7,0,0,0,},
    {1,2,3,4,7,0,0,0,},
    {0,1,2,3,4,7,0,0,},
    {5,7,0,0,0,0,0,0,},
    {0,5,7,0,0,0,0,0,},
    {1,5,7,0,0,0,0,0,},
    {0,1,5,7,0,0,0,0,},
    {2,5,7,0,0,0,0,0,},
    {0,2,5,7,0,0,0,0,},
    {1,2,5,7,0,0,0,0,},
    {0,1,2,5,7,0,0,0,},
    {3,5,7,0,0,0,0,0,},
    {0,3,5,7,0,0,0,0,},
    {1,3,5,7,0,0,0,0,},
    {0,1,3,5,7,0,0,0,},
    {2,3,5,7,0,0,0,0,},
    {0,2,3,5,7,0,0,0,},
    {1,2,3,5,7,0,0,0,},
    {0,1,2,3,5,7,0,0,},
    {4,5,7,0,0,0,0,0,},
    {0,4,5,7,0,0,0,0,},
    {1,4,5,7,0,0,0,0,},
    {0,1,4,5,7,0,0,0,},
    {2,4,5,7,0,0,0,0,},
    {0,2,4,5,7,0,0,0,},
    {1,2,4,5,7,0,0,0,},
    {0,1,2,4,5,7,0,0,},
    {3,4,5,7,0,0,0,0,},
    {0,3,4,5,7,0,0,0,},
    {1,3,4,5,7,0,0,0,},
    {0,1,3,4,5,7,0,0,},
    {2,3,4,5,7,0,0,0,},
    {0,2,3,4,5,7,0,0,},
    {1,2,3,4,5,7,0,0,},
    {0,1,2,3,4,5,7,0,},
    {6,7,0,0,0,0,0,0,},
    {0,6,7,0,0,0,0,0,},
    {1,6,7,0,0,0,0,0,},
    {0,1,6,7,0,0,0,0,},
    {2,6,7,0,0,0,0,0,},
    {0,2,6,7,0,0,0,0,},
    {1,2,6,7,0,0,0,0,},
    {0,1,2,6,7,0,0,0,},
    {3,6,7,0,0,0,0,0,},
    {0,3,6,7,0,0,0,0,},
    {1,3,6,7,0,0,0,0,},
    {0,1,3,6,7,0,0,0,},
    {2,3,6,7,0,0,0,0,},
    {0,2,3,6,7,0,0,0,},
    {1,2,3,6,7,0,0,0,},
    {0,1,2,3,6,7,0,0,},
    {4,6,7,0,0,0,0,0,},
    {0,4,6,7,0,0,0,0,},
    {1,4,6,7,0,0,0,0,},
    {0,1,4,6,7,0,0,0,},
    {2,4,6,7,0,0,0,0,},
    {0,2,4,6,7,0,0,0,},
    {1,2,4,6,7,0,0,0,},
    {0,1,2,4,6,7,0,0,},
    {3,4,6,7,0,0,0,0,},
    {0,3,4,6,7,0,0,0,},
    {1,3,4,6,7,0,0,0,},
    {0,1,3,4,6,7,0,0,},
    {2,3,4,6,7,0,0,0,},
    {0,2,3,4,6,7,0,0,},
    {1,2,3,4,6,7,0,0,},
    {0,1,2,3,4,6,7,0,},
    {5,6,7,0,0,0,0,0,},
    {0,5,6,7,0,0,0,0,},
    {1,5,6,7,0,0,0,0,},
    {0,1,5,6,7,0,0,0,},
    {2,5,6,7,0,0,0,0,},
    {0,2,5,6,7,0,0,0,},
    {1,2,5,6,7,0,0,0,},
    {0,1,2,5,6,7,0,0,},
    {3,5,6,7,0,0,0,0,},
    {0,3,5,6,7,0,0,0,},
    {1,3,5,6,7,0,0,0,},
    {0,1,3,5,6,7,0,0,},
    {2,3,5,6,7,0,0,0,},
    {0,2,3,5,6,7,0,0,},
    {1,2,3,5,6,7,0,0,},
    {0,1,2,3,5,6,7,0,},
    {4,5,6,7,0,0,0,0,},
    {0,4,5,6,7,0,0,0,},
    {1,4,5,6,7,0,0,0,},
    {0,1,4,5,6,7,0,0,},
    {2,4,5,6,7,0,0,0,},
    {0,2,4,5,6,7,0,0,},
    {1,2,4,5,6,7,0,0,},
    {0,1,2,4,5,6,7,0,},
    {3,4,5,6,7,0,0,0,},
    {0,3,4,5,6,7,0,0,},
    {1,3,4,5,6,7,0,0,},
    {0,1,3,4,5,6,7,0,},
    {2,3,4,5,6,7,0,0,},
    {0,2,3,4,5,6,7,0,},
    {1,2,3,4,5,6,7,0,},
    {0,1,2,3,4,5,6,7,},
};*/


byte_table *gen_byte_table(void) {
    byte_table *new_table = (byte_table *) malloc(sizeof(byte_table));
    new_table->nums = (uint8_t *) malloc(sizeof(uint8_t) * 256);
    new_table->pos = (uint8_t (*)[8]) malloc(sizeof(uint8_t) * 256 * 8);
    unsigned int num, byte, bit;
    for (byte = 0; byte < 256; byte++) {
        num = 0;
        for (bit = 0; bit < 8; bit++) {
            if (byte & (1 << bit)) {
                new_table->pos[byte][num] = bit;
                num++;
            }
        }
        new_table->nums[byte] = num;
    }
    return new_table;
}


unsigned int bits_between(uint8_t *bit_str, uint8_t a, uint8_t b) {
    unsigned int n_bits = 0;
    uint8_t byte_counts[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
    uint8_t min_bit, max_bit;
    
    if (a < b) {
        min_bit = a;
        max_bit = b;
    }
    else {
        max_bit = a;
        min_bit = b;
    }
    
    uint8_t mask;
    uint8_t min_byte = min_bit / 8;
    uint8_t max_byte = max_bit / 8;
    int same_byte = min_byte == max_byte;
    uint8_t byte_idx;
    uint8_t curr_int;
    
    byte_idx = min_bit / 8;
    min_bit %= 8;
    max_bit %= 8;
    if (same_byte) {
        mask = (1 << max_bit) - (1 << (min_bit + 1));
    }
    else {
        mask = 256 - (1 << (min_bit + 1));
    }
    curr_int = mask & bit_str[byte_idx];
    n_bits = byte_counts[curr_int & 15];
    curr_int >>= 4;
    n_bits += byte_counts[curr_int & 15];
    for (byte_idx++; byte_idx < max_byte; byte_idx++) {
        curr_int = bit_str[byte_idx];
        n_bits += byte_counts[curr_int & 15];
        curr_int >>= 4;
        n_bits += byte_counts[curr_int & 15];
    }
    if (!same_byte) {
        mask = (1 << max_bit) - 1;
        curr_int = mask & bit_str[byte_idx];
        n_bits += byte_counts[curr_int & 15];
        curr_int >>= 4;
        n_bits += byte_counts[curr_int & 15];
    }
    
    return n_bits;
}


uint8_t find_bits(const uint8_t *restrict bit_str, uint8_t *restrict bits, uint8_t n_bytes, const byte_table *tabl) {
    unsigned int n_bits = 0, byte_idx;
    uint8_t byte_bits, det_byte, bit_idx;
    for (byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        det_byte = bit_str[byte_idx];
        byte_bits = tabl->nums[det_byte];
        for (bit_idx = 0; bit_idx < byte_bits; bit_idx++) {
            bits[n_bits + bit_idx] = (8 * byte_idx + tabl->pos[det_byte][bit_idx]);
        }
        n_bits += byte_bits;
    }
    return n_bits;
}


void new_sorted(uint8_t *restrict orig_list, uint8_t *restrict new_list,
                uint8_t length, uint8_t del_idx, uint8_t new_el) {
    uint8_t offset = 0;
    uint8_t idx;
    if (new_el > orig_list[del_idx]) {
        memcpy(new_list, orig_list, sizeof(uint8_t) * del_idx);
        for (idx = del_idx + 1; idx < length; idx++) {
            if (orig_list[idx] < new_el) {
                offset++;
            }
        }
        memcpy(new_list + del_idx, orig_list + del_idx + 1, sizeof(uint8_t) * offset);
        new_list[del_idx + offset] = new_el;
        memcpy(new_list + del_idx + offset + 1, orig_list + del_idx + offset + 1, sizeof(uint8_t) * (length - del_idx - offset - 1));
    }
    else {
        for (idx = 0; idx < del_idx; idx++) {
            if (orig_list[idx] > new_el) {
                offset++;
            }
        }
        memcpy(new_list, orig_list, sizeof(uint8_t) * (del_idx - offset));
        new_list[del_idx - offset] = new_el;
        memcpy(new_list + del_idx - offset + 1, orig_list + del_idx - offset, sizeof(uint8_t) * offset);
        memcpy(new_list + del_idx + 1, orig_list + del_idx + 1, (length - del_idx - 1) * sizeof(uint8_t));
    }
}

void repl_sorted(uint8_t *srt_list, uint8_t length, uint8_t del_idx, uint8_t new_el) {
    uint8_t offset = 0;
    uint8_t idx;
    if (new_el > srt_list[del_idx]) {
        for (idx = del_idx + 1; idx < length; idx++) {
            if (srt_list[idx] < new_el) {
                offset++;
            }
        }
        memmove(srt_list + del_idx, srt_list + del_idx + 1, offset * sizeof(uint8_t));
        srt_list[del_idx + offset] = new_el;
    }
    else {
        for (idx = 0; idx < del_idx; idx++) {
            if (srt_list[idx] > new_el) {
                offset++;
            }
        }
        memmove(srt_list + del_idx - offset + 1, srt_list + del_idx - offset, offset * sizeof(uint8_t));
        srt_list[del_idx - offset] = new_el;
    }
}
