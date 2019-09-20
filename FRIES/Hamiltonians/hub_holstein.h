/*! \file
 *
 * \brief Utilities for Hubbard-Holstein in the site basis
 */

#ifndef hub_holstein_h
#define hub_holstein_h

#include <stdio.h>
#include <FRIES/fci_utils.h>
#include <FRIES/math_utils.h>
#include <FRIES/Ext_Libs/dcmt/dc.h>


#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Uniformly chooses num_sampl excitations from a column of the
 *  Hamiltonian using independent multinomial sampling.
 *
 * \param [in] det          bit string representation of the origin determinant
 * \param [in] n_elec       number of electrons in the system
 * \param [in] neighbors    list of occupied orbitals with unoccupied
 *                          neighboring orbitals, generated by find_neighbors()
 * \param [in] num_sampl    number of excitations to sample
 * \param [in] rn_ptr       Pointer to an mt_struct object for RN generation
 * \param [out] chosen_orbs Array of occupied (0th column) and unoccupied (1st
 *                          column) orbitals for each excitation
 *                          (dimensions \p num_sampl x 2)
 */
void hub_multin(long long det, unsigned int n_elec,
                unsigned char neighbors[][n_elec + 1],
                unsigned int num_sampl, mt_struct *rn_ptr,
                unsigned char chosen_orbs[][2]);


/*! \brief Generate all nonzero elements in a column of the Hamiltonian
 *  (corresponding to excitations from a particular determinant)
 *
 * \param [in] det          bit string representation of the origin determinant
 * \param [in] n_elec       number of electrons in the system
 * \param [in] neighbors    list of occupied orbitals with unoccupied
 *                          neighboring orbitals, generated by find_neighbors()
 * \param [out] chosen_orbs Array of occupied (0th column) and unoccupied (1st
 *                          column) orbitals for each excitation
 * \return number of excitations
 */
size_t hub_all(long long det, unsigned int n_elec,
               unsigned char neighbors[][n_elec + 1],
               unsigned char chosen_orbs[][2]);


/*! \brief Calculate the diagonal matrix element corresponding to a determinant,
 *  in units of U
 *
 * The value of the diagonal matrix element is the number of sites occupied with
 * both an up and down electron
 *
 * \param [in] det          bit string representation of the determinant
 * \param [in] n_sites      number of sites along one dimension of the lattice
 * \param [in] table        byte table struct generated by gen_byte_table()
 * \return number of spin-up & spin-down orbitals that overlap
 */
unsigned int hub_diag(long long det, unsigned int n_sites, byte_table *table);


/*! \brief Generate the bit string for the Neel state
 *
 * \param [in] n_sites      number of sites along one dimension of the lattice
 * \param [in] n_elec       number of electrons in the system
 * \param [in] n_dim        number of dimensions in the lattice
 * \return bit string representation of the determinant
 */
long long gen_neel_det_1D(unsigned int n_sites, unsigned int n_elec,
                          unsigned int n_dim);


/*! \brief Identify all determinants in a vector connected via H to a reference
 *  determinant and sum their respective off-diagonal matrix elements
 *
 * \param [in] dets         determinant indices of vector elements
 * \param [in] vals         values of vector elements
 * \param [in] n_dets       number of elements in the vector
 * \param [in] ref_det      reference determinant
 * \return sum of all (off-diagonal) matrix elements
 */
double calc_ref_ovlp(long long *dets, void *vals, size_t n_dets, long long ref_det,
                     byte_table *table, dtype type);

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
                 unsigned char (*neighbors)[n_elec + 1], unsigned char *orbs);

#ifdef __cplusplus
}
#endif

#endif /* hub_holstein_h */
