/*! \file
 *
 * \brief Utilities for manipulating Slater determinants for FCI calculations
 */

#include "fci_utils.h"
#include <FRIES/det_store.h>
#include <nmmintrin.h>

void gen_hf_bitstring(unsigned int n_orb, unsigned int n_elec, uint8_t *det) {
    uint8_t byte_idx;
    for (byte_idx = 0; byte_idx < CEILING(2 * n_orb, 8); byte_idx++) {
        det[byte_idx] = 0;
    }
    for (byte_idx = 0; byte_idx < (n_elec / 2 / 8); byte_idx++) {
        det[byte_idx] = 255;
    }
    if ((n_elec / 2 % 8) > 0) {
        det[n_elec / 2 / 8] = (1 << (n_elec / 2 % 8)) - 1;
    }
    
    for (byte_idx = n_elec / 2 / 8 + 1; byte_idx <= n_orb / 8; byte_idx++) {
        det[byte_idx] = 0;
    }
    
    byte_idx = n_orb / 8;
    uint8_t n_elec_mid = 8 - (n_orb % 8);
    if (n_elec_mid > n_elec / 2) {
        n_elec_mid = n_elec / 2;
    }
    uint8_t elec_mid = (1 << ((n_orb % 8) + n_elec_mid)) - (1 << (n_orb % 8));
    det[byte_idx] |= elec_mid;
    byte_idx++;
    for (; byte_idx < ((n_orb + n_elec / 2) / 8); byte_idx++) {
        det[byte_idx] = 255;
    }
    byte_idx = (n_orb + n_elec / 2) / 8;
    det[byte_idx] |= (1 << ((n_orb + n_elec / 2) % 8)) - (1 << ((n_orb / 8 == byte_idx) ? (n_orb % 8) : 0));
    
    for (byte_idx++; byte_idx < CEILING(2 * n_orb, 8); byte_idx++) {
        det[byte_idx] = 0;
    }
}


int sing_det_parity(uint8_t *det, uint8_t *orbs) {
    zero_bit(det, orbs[0]);
    int sign = excite_sign(orbs[0], orbs[1], det);
    set_bit(det, orbs[1]);
    return sign;
}


int doub_det_parity(uint8_t *det, uint8_t *orbs) {
    zero_bit(det, orbs[0]);
    zero_bit(det, orbs[1]);
    int sign = excite_sign(orbs[2], orbs[0], det);
    sign *= excite_sign(orbs[3], orbs[1], det);
    set_bit(det, orbs[2]);
    set_bit(det, orbs[3]);
    return sign;
}


void doub_ex_orbs(uint8_t *curr_orbs, uint8_t *new_orbs, uint8_t *ex_orbs,
                  uint8_t n_elec) {
    uint8_t spin_shift1 = (ex_orbs[0] / (n_elec / 2)) * n_elec / 2;
    uint8_t spin_shift2 = (ex_orbs[1] / (n_elec / 2)) * n_elec / 2;
    new_sorted(curr_orbs + spin_shift1, new_orbs + spin_shift1, n_elec / 2, ex_orbs[0] - spin_shift1, ex_orbs[2]);
    if (spin_shift1 == spin_shift2) {
        memcpy(new_orbs + n_elec / 2 - spin_shift1, curr_orbs + n_elec / 2 - spin_shift1, n_elec / 2);
        repl_sorted(new_orbs + spin_shift1, n_elec / 2, ex_orbs[1] - spin_shift1, ex_orbs[3]);
    }
    else {
        new_sorted(curr_orbs + spin_shift2, new_orbs + spin_shift2, n_elec / 2, ex_orbs[1] - spin_shift2, ex_orbs[3]);
    }
}

int excite_sign(uint8_t cre_op, uint8_t des_op, uint8_t *det) {
    unsigned int n_perm = bits_between(det, cre_op, des_op);
    if (n_perm % 2 == 0)
        return 1;
    else
        return -1;
}

uint8_t find_nth_virt(uint8_t *occ_orbs, int spin, uint8_t n_elec,
                      uint8_t n_orb, uint8_t n) {
    uint8_t virt_orb = n_orb * spin + n;
    size_t orb_idx;
    for (orb_idx = n_elec / 2 * spin; occ_orbs[orb_idx] <= virt_orb && orb_idx < n_elec; orb_idx++) {
        if (occ_orbs[orb_idx] <= virt_orb) {
            virt_orb++;
        }
    }
    return virt_orb;
}

void sing_ex_orbs(uint8_t *restrict curr_orbs, uint8_t *restrict new_orbs,
                  uint8_t *ex_orbs, uint8_t n_elec) {
    uint8_t spin_shift = (ex_orbs[0] / (n_elec / 2)) * n_elec / 2;
    new_sorted(curr_orbs + spin_shift, new_orbs + spin_shift, n_elec / 2, ex_orbs[0] - spin_shift, ex_orbs[1]);
    memcpy(new_orbs + n_elec / 2 - spin_shift, curr_orbs + n_elec / 2 - spin_shift, n_elec / 2);
}


void flip_spins(uint8_t *det_in, uint8_t *det_out, uint8_t n_orb) {
    uint8_t mid_byte_idx = n_orb / 8;
    uint8_t mid_bit_offset = n_orb % 8;
    uint8_t n_bytes = CEILING(2 * n_orb, 8);
    det_out[mid_byte_idx] = 0;
    for (uint8_t byte_idx = 0; byte_idx < mid_byte_idx + (n_orb % 8 != 0); byte_idx++) {
        det_out[byte_idx] = det_in[byte_idx + mid_byte_idx] >> mid_bit_offset;
        if (mid_bit_offset > 0) {
            det_out[byte_idx] |= det_in[byte_idx + mid_byte_idx + 1] << (8 - mid_bit_offset);
        }
    }
    det_out[mid_byte_idx] |= det_in[0] << mid_bit_offset;
    for (uint8_t byte_idx = mid_byte_idx + 1; byte_idx < n_bytes; byte_idx++) {
        det_out[byte_idx] = det_in[byte_idx - mid_byte_idx - 1] >> (8 - mid_bit_offset);
        det_out[byte_idx] |= det_in[byte_idx - mid_byte_idx] << mid_bit_offset;
    }
}


uint8_t find_excitation(const uint8_t *str1, const uint8_t *str2, uint8_t *orbs, uint8_t n_bytes) {
    uint8_t n_bits = 0;
    uint8_t byte_idx;
    __m128i bit_offset_v = _mm_set_epi8(8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0);
    __m128i sixteen = _mm_set1_epi8(16);
    // Find bits that are occupied in str1 but unoccupied in str2
    for (byte_idx = 0; byte_idx < n_bytes - 1; byte_idx += 2) {
        uint8_t byte1 = (str1[byte_idx] ^ str2[byte_idx]) & str1[byte_idx];
        uint8_t byte1_nbits = _mm_popcnt_u32(byte1);
        
        uint8_t byte2 = (str1[byte_idx + 1] ^ str2[byte_idx + 1]) & str1[byte_idx + 1];
        uint8_t byte2_nbits = _mm_popcnt_u32(byte2);
        
        uint8_t bytes_nbits = byte1_nbits + byte2_nbits;
        
        if (bytes_nbits == 0) {
            continue;
        }
        if (n_bits + bytes_nbits > 2) {
            return UINT8_MAX;
        }
        
        __m128i bit_vec = _mm_set_epi64x(byte_pos[byte2], byte_pos[byte1]);
        bit_vec += bit_offset_v;
        uint8_t *vec_ptr = (uint8_t *)&bit_vec;
        
        memcpy(orbs + n_bits, vec_ptr, byte1_nbits);
        n_bits += byte1_nbits;
        memcpy(orbs + n_bits, vec_ptr + 8, byte2_nbits);
        n_bits += byte2_nbits;
        
        bit_offset_v += sixteen;
    }
    if (byte_idx < n_bytes) {
        uint8_t byte = (str1[byte_idx] ^ str2[byte_idx]) & str1[byte_idx];
        uint8_t byte_nbits = _mm_popcnt_u32(byte);
        
        if (byte_nbits > 0) {
            if (byte_nbits + n_bits > 2) {
                return UINT8_MAX;
            }
            __m128i bit_vec = _mm_set_epi64x(0, byte_pos[byte]);
            bit_vec += bit_offset_v;
            uint8_t *vec_ptr = (uint8_t *)&bit_vec;

            memcpy(orbs + n_bits, vec_ptr, byte_nbits);
            n_bits += byte_nbits;
        }
    }
    
    if (n_bits == 0) { // all occupied orbitals are the same, so we can assume virtuals are too
        return 0;
    }
    
    uint8_t limit = n_bits * 2;
    bit_offset_v = _mm_set_epi8(8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0);
    
    // Find bits that are unoccupied in str1 but occupied in str2
    for (byte_idx = 0; byte_idx < n_bytes - 1; byte_idx += 2) {
        uint8_t byte1 = (str1[byte_idx] ^ str2[byte_idx]) & str2[byte_idx];
        uint8_t byte1_nbits = _mm_popcnt_u32(byte1);
        
        uint8_t byte2 = (str1[byte_idx + 1] ^ str2[byte_idx + 1]) & str2[byte_idx + 1];
        uint8_t byte2_nbits = _mm_popcnt_u32(byte2);
        
        uint8_t tot_nbits = byte1_nbits + byte2_nbits;
        
        if (tot_nbits == 0) {
            continue;
        }
        
        __m128i bit_vec = _mm_set_epi64x(byte_pos[byte2], byte_pos[byte1]);
        bit_vec += bit_offset_v;
        uint8_t *vec_ptr = (uint8_t *)&bit_vec;
        
        memcpy(orbs + n_bits, vec_ptr, byte1_nbits);
        n_bits += byte1_nbits;
        memcpy(orbs + n_bits, vec_ptr + 8, byte2_nbits);
        n_bits += byte2_nbits;
        
        bit_offset_v += sixteen;
        
        if (n_bits == limit) {
            return limit / 2;
        }
    }
    if (byte_idx < n_bytes) {
        uint8_t byte = (str1[byte_idx] ^ str2[byte_idx]) & str2[byte_idx];
        uint8_t byte_nbits = _mm_popcnt_u32(byte);
        
        if (byte_nbits > 0) {
            __m128i bit_vec = _mm_set_epi64x(0, byte_pos[byte]);
            bit_vec += bit_offset_v;
            uint8_t *vec_ptr = (uint8_t *)&bit_vec;

            memcpy(orbs + n_bits, vec_ptr, byte_nbits);
            n_bits += byte_nbits;
        }
    }
    
    return n_bits / 2;
}
