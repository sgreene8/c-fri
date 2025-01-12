/*! \file 
 *
 * \brief Miscellaneous math utilities and definitions
 */

#ifndef math_utils_h
#define math_utils_h

#include <string.h>
#include <stdint.h>
#include <nmmintrin.h>

#define CEILING(x,y) ((x + y - 1) / y)
#define SIGN(x) ((x > 0) - (x < 0))
#define TRI_N(n)((n) * (n + 1) / 2)
#define I_J_TO_TRI_NODIAG(i, j)(TRI_N(j - 1) + i)
#define I_J_TO_TRI_WDIAG(i, j)(TRI_N(j) + i)

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t byte_pos[256] = {0x0, 0x0, 0x1, 0x100, 0x2, 0x200, 0x201, 0x20100, 0x3, 0x300, 0x301, 0x30100, 0x302, 0x30200, 0x30201, 0x3020100, 0x4, 0x400, 0x401, 0x40100, 0x402, 0x40200, 0x40201, 0x4020100, 0x403, 0x40300, 0x40301, 0x4030100, 0x40302, 0x4030200, 0x4030201, 0x403020100, 0x5, 0x500, 0x501, 0x50100, 0x502, 0x50200, 0x50201, 0x5020100, 0x503, 0x50300, 0x50301, 0x5030100, 0x50302, 0x5030200, 0x5030201, 0x503020100, 0x504, 0x50400, 0x50401, 0x5040100, 0x50402, 0x5040200, 0x5040201, 0x504020100, 0x50403, 0x5040300, 0x5040301, 0x504030100, 0x5040302, 0x504030200, 0x504030201, 0x50403020100, 0x6, 0x600, 0x601, 0x60100, 0x602, 0x60200, 0x60201, 0x6020100, 0x603, 0x60300, 0x60301, 0x6030100, 0x60302, 0x6030200, 0x6030201, 0x603020100, 0x604, 0x60400, 0x60401, 0x6040100, 0x60402, 0x6040200, 0x6040201, 0x604020100, 0x60403, 0x6040300, 0x6040301, 0x604030100, 0x6040302, 0x604030200, 0x604030201, 0x60403020100, 0x605, 0x60500, 0x60501, 0x6050100, 0x60502, 0x6050200, 0x6050201, 0x605020100, 0x60503, 0x6050300, 0x6050301, 0x605030100, 0x6050302, 0x605030200, 0x605030201, 0x60503020100, 0x60504, 0x6050400, 0x6050401, 0x605040100, 0x6050402, 0x605040200, 0x605040201, 0x60504020100, 0x6050403, 0x605040300, 0x605040301, 0x60504030100, 0x605040302, 0x60504030200, 0x60504030201, 0x6050403020100, 0x7, 0x700, 0x701, 0x70100, 0x702, 0x70200, 0x70201, 0x7020100, 0x703, 0x70300, 0x70301, 0x7030100, 0x70302, 0x7030200, 0x7030201, 0x703020100, 0x704, 0x70400, 0x70401, 0x7040100, 0x70402, 0x7040200, 0x7040201, 0x704020100, 0x70403, 0x7040300, 0x7040301, 0x704030100, 0x7040302, 0x704030200, 0x704030201, 0x70403020100, 0x705, 0x70500, 0x70501, 0x7050100, 0x70502, 0x7050200, 0x7050201, 0x705020100, 0x70503, 0x7050300, 0x7050301, 0x705030100, 0x7050302, 0x705030200, 0x705030201, 0x70503020100, 0x70504, 0x7050400, 0x7050401, 0x705040100, 0x7050402, 0x705040200, 0x705040201, 0x70504020100, 0x7050403, 0x705040300, 0x705040301, 0x70504030100, 0x705040302, 0x70504030200, 0x70504030201, 0x7050403020100, 0x706, 0x70600, 0x70601, 0x7060100, 0x70602, 0x7060200, 0x7060201, 0x706020100, 0x70603, 0x7060300, 0x7060301, 0x706030100, 0x7060302, 0x706030200, 0x706030201, 0x70603020100, 0x70604, 0x7060400, 0x7060401, 0x706040100, 0x7060402, 0x706040200, 0x706040201, 0x70604020100, 0x7060403, 0x706040300, 0x706040301, 0x70604030100, 0x706040302, 0x70604030200, 0x70604030201, 0x7060403020100, 0x70605, 0x7060500, 0x7060501, 0x706050100, 0x7060502, 0x706050200, 0x706050201, 0x70605020100, 0x7060503, 0x706050300, 0x706050301, 0x70605030100, 0x706050302, 0x70605030200, 0x70605030201, 0x7060503020100, 0x7060504, 0x706050400, 0x706050401, 0x70605040100, 0x706050402, 0x70605040200, 0x70605040201, 0x7060504020100, 0x706050403, 0x70605040300, 0x70605040301, 0x7060504030100, 0x70605040302, 0x7060504030200, 0x7060504030201, 0x706050403020100};


typedef enum {
    DOUB,
    INT
} dtype;


/*! \brief Generate a list of the indices of the nonzero bits in a bit string
 *
 * \param [in] bit_str      The bit string to decode
 * \param [out] bits        The array in which to store the bit indices
 * \param [in] n_bytes      The length of \p bit_str
 */
uint8_t find_bits(const uint8_t *bit_str, uint8_t *bits, uint8_t n_bytes);

/*! \brief Generate a list of the indices of the first 4 bits that differ between two bit strings
 *
 * \param [in] str1     First bit string
 * \param [in] str2     Second bit string
 * \param [out] bits    List of up to 4 indices bits that differ
 * \param [in] n_bytes  Number of bytes to compare in the bit strings
 * \return The number of bits by which the bit strings differ, or UINT8_MAX if greater than 4
 */
uint8_t find_diff_bits(const uint8_t *str1, const uint8_t *str2, uint8_t *bits, uint8_t n_bytes);

/*! \brief Count number of 1's between two bits in binary representation of a
 * number
 *
 * The order of the two bit indices does not matter
 *
 * \param [in] bit_str  The binary representation of the number in bit string format
 * \param [in] a        The position of one of the bits in question
 * \param [in] b        The position of the second bit in question
 * \return the number of bits
 */
unsigned int bits_between(uint8_t *bit_str, uint8_t a, uint8_t b);


/*! \brief Given an ordered list of numbers, return a copy, modified such that the element at a specified index is
 * replaced with a new element and re-sorted
 *
 * \param [in] orig_list     The original ordered list of numbers
 * \param [out] new_list    The resulting, new list
 * \param [in] length   The number of elements in the original and new lists
 * \param [in]  del_idx     The index of the item to be replaced
 * \param [in]  new_el      The element replacing the removed element
 */
void new_sorted(uint8_t *orig_list, uint8_t *new_list,
                uint8_t length, uint8_t del_idx, uint8_t new_el);


/*! \brief Given an ordered list of numbers, replace an element at a specified index with a new element and re-sort
 *
 * \param [in] srt_list     The original ordered list of numbers
 * \param [in] length   The number of elements in the original and new lists
 * \param [in]  del_idx     The index of the item to be replaced
 * \param [in]  new_el      The element replacing the removed element
 */
 void repl_sorted(uint8_t *srt_list, uint8_t length, uint8_t del_idx, uint8_t new_el);


#ifdef __cplusplus
}
#endif

#endif /* math_utils_h */
