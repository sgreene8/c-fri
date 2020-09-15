/*! \file 
 *
 * \brief Miscellaneous math utilities and definitions
 */

#ifndef math_utils_h
#define math_utils_h

#include <string.h>
#include <stdint.h>

#define CEILING(x,y) ((x + y - 1) / y)
#define SIGN(x) ((x > 0) - (x < 0))

#ifdef __cplusplus
extern "C" {
#endif

static const uint8_t byte_nums[256] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,};

static const uint8_t byte_pos[256][8] = {
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
};


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
