/*! \file
 *
 * \brief Utilities for Hubbard-Holstein in the site basis
 */

#ifndef hub_holstein_h
#define hub_holstein_h

#include <cstdio>
#define __STDC_LIMIT_MACROS
#include <cstdint>
#include <FRIES/fci_utils.h>
#include <FRIES/math_utils.h>
#include <random>
#include <FRIES/det_store.h>
#include <FRIES/ndarr.hpp>
#include <stdexcept>


/*! \brief Uniformly chooses num_sampl excitations from a column of the
 *  Hamiltonian using independent multinomial sampling.
 *
 * \param [in] n_elec       number of electrons in the system
 * \param [in] neighbors    list of occupied orbitals with unoccupied
 *                          neighboring orbitals, generated by find_neighbors()
 * \param [in] num_sampl    number of excitations to sample
 * \param [in] mt_obj       Reference to an initialized MT object for RN generation
 * \param [out] chosen_orbs Array of occupied (0th column) and unoccupied (1st
 *                          column) orbitals for each excitation
 *                          (dimensions \p num_sampl x 2)
 */
void hub_multin(unsigned int n_elec, const uint8_t *neighbors,
                unsigned int num_sampl, std::mt19937 &mt_obj,
                uint8_t chosen_orbs[][2]);


/*! \brief Generate all nonzero off-diagonal Hubbard elements in a column of the Hamiltonian
 *  (corresponding to excitations from a particular determinant)
 *
 * \param [in] n_elec       number of electrons in the system
 * \param [in] neighbors    list of occupied orbitals with unoccupied
 *                          neighboring orbitals, generated by find_neighbors()
 * \param [out] chosen_orbs Array of occupied (0th column) and unoccupied (1st
 *                          column) orbitals for each excitation
 * \return number of excitations
 */
size_t hub_all(unsigned int n_elec, uint8_t *neighbors,
               uint8_t chosen_orbs[][2]);


/*! \brief Calculate the diagonal matrix element corresponding to a determinant,
 *  in units of U
 *
 * The value of the diagonal matrix element is the number of sites occupied with
 * both an up and down electron
 *
 * \param [in] det          bit string representation of the determinant
 * \param [in] n_sites      number of sites along one dimension of the lattice
 * \return number of spin-up & spin-down orbitals that overlap
 */
unsigned int hub_diag(uint8_t *det, unsigned int n_sites);


/*! \brief Generate the bit string for the Neel state
 *
 * All phonon bits are zeroed
 *
 * \param [in] n_sites      number of sites along one dimension of the lattice
 * \param [in] n_elec       number of electrons in the system
 * \param [in] ph_bits      Number of bits designated to represent the phonon occupation number for each site
 * \param [out] det         Upon return, contains bit string representation of the neel determinant
 */
void gen_neel_det_1D(unsigned int n_sites, unsigned int n_elec, uint8_t ph_bits,
                     uint8_t *det);


/*! \brief Identify all determinants in a vector connected via H to a reference
 *  determinant and sum their respective off-diagonal matrix elements
 *
 *  It is assumed that the reference state has no phonons
 *
 * \param [in] dets         determinant indices of vector elements
 * \param [in] vals         values of vector elements
 * \param [in] phonons      Phonon occupation numbers for each basis element index
 * \param [in] n_dets       number of elements in the vector
 * \param [in] ref_det      reference determinant
 * \param [in] occ_ref      List of occupied orbitals in the reference determinant
 * \param [in] n_elec       Number of electrons represented in each determinant
 * \param [in] n_sites      Number of sites in the Hubbard lattice
 * \param [in] g_over_t     Ratio of the electron-phonon coupling parameter to t, the Hubbard hopping
 * \return sum of all (off-diagonal) matrix elements over t, the Hubbard hopping
 */
template <typename T>
double calc_ref_ovlp(Matrix<uint8_t> &dets, T *vals, Matrix<uint8_t> &phonons,
                size_t n_dets, uint8_t *ref_det, uint8_t *occ_ref,
                uint8_t n_elec, unsigned int n_sites, double g_over_t) {
    size_t det_idx, byte_idx;
    double result = 0;
    uint8_t *curr_det;
    for (det_idx = 0; det_idx < n_dets; det_idx++) {
        curr_det = dets[det_idx];
        
        // Test whether the electronic part of the current state is equivalent to
        // that of the reference
        uint8_t last_elec_byte = CEILING(2 * n_sites, 8);
        uint8_t last_elec_mask = (1 << (2 * n_sites % 8)) - 1;
        int elec_same = !memcmp(curr_det, ref_det, 2 * n_sites / 8) && (curr_det[last_elec_byte - 1] & last_elec_mask) == (ref_det[last_elec_byte - 1] & last_elec_mask);
        if (elec_same) {
            uint8_t sites_found = 0;
            uint8_t site_elecs = 0;
            for (uint8_t site = 0; site < n_sites && sites_found < 2; site++) {
                uint8_t ph_num = phonons(det_idx, site);
                uint8_t n_occ = read_bit(ref_det, site) + read_bit(ref_det, site + n_sites);
                if (ph_num > 1 || (ph_num == 1 && n_occ == 0)) {
                    site_elecs = 0;
                    break;
                }
                else if (ph_num == 1) {
                    site_elecs = n_occ;
                    sites_found++;
                }
            }
            if (sites_found == 2) {
                site_elecs = 0;
            }
            result -= vals[det_idx] * g_over_t * site_elecs;
        }
        else {
            uint8_t num_ph = 0;
            for (uint8_t site_idx = 0; site_idx < n_sites; site_idx++) {
                num_ph += phonons(det_idx, site_idx);
            }
            if (num_ph != 0) {
                continue;
            }
            // Count the number of bits in curr_det that are
            // 1) not occupied in ref_det
            // 2) have an occupied bit to the left in ref_det AND don't have an occupied bit to the left in curr_det
            // OR
            // have an occupied bit to the right in ref_det AND don't have an occupied bit to the right in curr_det
            // if this number is 1, and all other bits are the same, then it is connected
            uint8_t n_hop = 0;
            uint8_t n_common = 0;
            uint8_t mask;
            for (byte_idx = 0; byte_idx < CEILING(2 * n_sites, 8) && n_hop <= 1; byte_idx++) {
                uint8_t not_occ = curr_det[byte_idx] & ~ref_det[byte_idx];
                uint8_t ref_left = curr_det[byte_idx] & (ref_det[byte_idx] >> 1);
                uint8_t not_occ_left = ~curr_det[byte_idx] >> 1;
                uint8_t ref_right = curr_det[byte_idx] & (ref_det[byte_idx] << 1);
                uint8_t not_occ_right = ~curr_det[byte_idx] << 1;
                
                if (byte_idx > 0) {
                    ref_right |= curr_det[byte_idx] & ((ref_det[byte_idx - 1] >> 7) & 1);
                    not_occ_right |= (~curr_det[byte_idx - 1] >> 7) & 1;
                }
                if (byte_idx < CEILING(2 * n_sites, 8) - 1) {
                    ref_left |= curr_det[byte_idx] & (ref_det[byte_idx + 1] << 7);
                    not_occ_left |= ~curr_det[byte_idx + 1] << 7;
                }
                if (byte_idx == CEILING(n_sites, 8)) {
                    ref_left &= ~(1 << ((n_sites - 1) % 8)); // open boundary conditions
                }
                mask = not_occ & ((ref_left & not_occ_left) | (ref_right & not_occ_right));
                
                if (byte_idx == (CEILING(2 * n_sites, 8) - 1) && (2 * n_sites % 8 != 0)) {
                    mask &= (1 << (2 * n_sites % 8)) - 1;
                }
                
                n_hop += byte_nums[mask];
                if (n_hop > 1) {
                    break;
                }
                
                n_common += byte_nums[ref_det[byte_idx] & curr_det[byte_idx]];
            }
            if (n_hop == 1 && n_common == (n_elec - 1)) {
                result += vals[det_idx];
            }
        }
    }
    return result;
}


/*! \brief Find the orbitals involved in the ith excitation from a Hubbard
 * determinant
 *
 * \param [in] chosen_idx   The index of the excitation in question
 * \param [in] n_elec       Number of electrons defining the Hubbard determinant
 * \param [in] neighbors    2-D array indicating which occupied orbitals in the
 *                          determinant have an empty neighboring orbital
 *                          (dimensions 2 x (n_elec + 1))
 * \param [out] orbs        The orbitals defining the excitation
 */
void idx_to_orbs(unsigned int chosen_idx, unsigned int n_elec,
                 const uint8_t *neighbors, uint8_t *orbs);


/*! \brief Find the ith doubly occupied site in a Hubbard determinant
 * \param [in] chosen_idx       The index of the desired site
 * \param [in] n_elec       Number of electrons defining the Hubbard determinant
 * \param [in] occ      List of occupied sites in this determinant
 * \param [in] det      Bit-string representation of determinant
 * \param [in] n_sites      Number of sites in the Hubbard lattice
 * \return The site index of the orbital, or 255 if not found
 */
uint8_t idx_of_doub(unsigned int chosen_idx, unsigned int n_elec,
                    const uint8_t *occ, const uint8_t *det, unsigned int n_sites);


/*! \brief Find the ith singly occupied site in a Hubbard determinant
 * \param [in] chosen_idx       The index of the desired site
 * \param [in] n_elec       Number of electrons defining the Hubbard determinant
 * \param [in] occ      List of occupied sites in this determinant
 * \param [in] det      Bit-string representation of determinant
 * \param [in] n_sites      Number of sites in the Hubbard lattice
 * \return The site index of the orbital, or 255 if not found
 */
uint8_t idx_of_sing(unsigned int chosen_idx, unsigned int n_elec,
                    const uint8_t *occ, const uint8_t *det, unsigned int n_sites);

#endif /* hub_holstein_h */
