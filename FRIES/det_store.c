/*! \file
 *
 * \brief Utilities for keeping track of Slater determinant indices of a
 * sparse vector.
 */

#include "det_store.h"
#include <stdio.h>


int bit_str_equ(uint8_t *str1, uint8_t *str2, uint8_t n_bytes) {
    int byte_idx;
    int result = 0;
    for (byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        result += (str1[byte_idx] != str2[byte_idx]);
    }
    return result == 0;
}


void zero_bit(uint8_t *bit_str, uint8_t bit_idx) {
    uint8_t *byte = &bit_str[bit_idx / 8];
    uint8_t mask = ~0 ^ (1 << (bit_idx % 8));
    *byte &= mask;
}

void set_bit(uint8_t *bit_str, uint8_t bit_idx) {
    uint8_t *byte = &bit_str[bit_idx / 8];
    uint8_t mask = 1 << (bit_idx % 8);
    *byte |= mask;
}

void print_str(uint8_t *bit_str, uint8_t n_bytes, char *out_str) {
    uint8_t byte_idx;
    for (byte_idx = n_bytes; byte_idx > 0; byte_idx--) {
        sprintf(&out_str[(n_bytes - byte_idx) * 2], "%02x", bit_str[byte_idx - 1]);
    }
    out_str[n_bytes * 2] = '\0';
}
