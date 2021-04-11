/*! \file
 *
 * \brief Utilities for Near-uniform compression of Hamiltonian
 *
 * The elements of the intermediate matrices in the Near-Uniform scheme are
 * defined based on symmetry relationships among the orbitals in the HF basis.
 * This file therefore also contains functions for creating lists of
 * symmetry-allowed orbitals
 */

#ifndef near_uniform_h
#define near_uniform_h

#include <FRIES/ndarr.hpp>
#include <random>
#include <FRIES/Hamiltonians/molecule.hpp>


/*! \brief A pair of orbitals involved in a double excitation */
struct orb_pair {
    uint8_t orb1; ///< The first spinorbital, possible values range from 0 to 2 * n_orb - 1
    uint8_t orb2; ///< The second spinorbital, possible values range from 0 to 2 * n_orb - 1
    uint8_t spin1; ///< Spin of the first orbital (0 or 1)
    uint8_t spin2; ///< Spin of the second orbital (0 or 1)
};



/*! \brief Count number of unoccupied orbitals in a particular determinant with
 * each irrep
 *
 * \param [out] counts      Table containing numbers of unoccupied orbitals. The
 *                          row index is the irrep index, and the column index
 *                          is the spin index
                            (dimensions n_irreps x 2)
 * \param [in] occ_orbs     Array of occupied orbitals in the determinant
 *                          (length \p n_elec)
 * \param [in] n_elec       Number of occupied orbitals in the determinant
 * \param [in] symm     Object containing information about the symmetries of orbitals in the single-particle basis 
 */
void count_symm_virt(unsigned int counts[][2], uint8_t *occ_orbs,
                     unsigned int n_elec, SymmInfo *symm);


/*! \brief Execute binomial sampling with n samples, each with a probability p of
 * success.
 *
 * \param [in] n            Number of samples to draw
 * \param [in] p            Probability of a successful event for each sample
 * \param [in] mt_obj       Reference to an initialized MT object for RN generation
 * \return number of successes
 */
unsigned int bin_sample(unsigned int n, double p, std::mt19937 &mt_obj);


/*! \brief Perform multinomial sampling on the double excitation part of the
 *  Hamiltonian matrix using the near-uniform factorization
 *
 * Determinants are chosen according to the symmetry-adapted rules described in
 * Sec. 5.2 of Booth et al. (2014) using independent multinomial sampling.
 *
 * \param [in] det          bit string representation of the origin determinant
 *                          defining the respective column of the Hamiltonian
 * \param [in] occ_orbs     list of occupied orbitals in det
 *                          (length \p num_elec)
 * \param [in] num_elec     number of occupied orbitals in det
 * \param [in] symm     Object containing information about the symmetries of orbitals in the single-particle basis
 * \param [in] unocc_sym_counts List of unoccupied orbitals with each irrep,
 *                          as generated by count_symm_virt()
 *                          dimensions (n_irreps x 2)
 * \param [in] num_sampl    number of double excitations to sample from the
 *                          column
 * \param [in] mt_obj       Reference to an initialized MT object for RN generation
 * \param [out] chosen_orbs Contains occupied (0th and 1st columns) and
 *                          unoccupied (2nd and 3rd columns) orbitals for each
 *                          excitation (dimensions num_sampl x 4)
 * \param [out] prob_vec    Contains probability for each excitation
 *                          (length num_sampl)
 * \return number of excitations sampled (less than num_sampl if some excitations
 *  were null)
 */
unsigned int doub_multin(uint8_t *det, uint8_t *occ_orbs,
                         unsigned int num_elec, SymmInfo *symm,
                         unsigned int (* unocc_sym_counts)[2],
                         unsigned int num_sampl,
                         std::mt19937 &mt_obj, uint8_t (* chosen_orbs)[4],
                         double *prob_vec);


/*! \brief Perform multinomial sampling on the single excitation part of the
 *  Hamiltonian matrix using the near-uniform factorization
 *
 * Determinants are chosen according to the symmetry-adapted rules described in
 * Sec. 5.2 of Booth et al. (2014) using independent multinomial sampling.
 *
 * \param [in] det          bit string representation of the origin determinant
 *                          defining the respective column of the Hamiltonian
 * \param [in] occ_orbs     list of occupied orbitals in \p det
 *                          (length \p num_elec)
 * \param [in] num_elec     number of occupied orbitals in \p det
 * \param [in] symm     Object containing information about the symmetries of orbitals in the single-particle basis
 * \param [in] unocc_sym_counts List of unoccupied orbitals with each irrep,
 *                          as generated by count_symm_virt()
 *                          dimensions (n_irreps x 2)
 * \param [in] num_sampl    number of double excitations to sample from the
 *                          column
 * \param [in] mt_obj       Reference to an initialized MT object for RN generation
 * \param [out] chosen_orbs Contains occupied (0th column) and unoccupied
 *                          (1st column) orbitals for each excitation
 *                          (dimensions \p num_sampl x 2)
 * \param [out] prob_vec    Contains probability for each excitation
 *                          (length num_sampl)
 * \return number of excitations sampled (less than num_sampl if some excitations
 *  were null)
 */
unsigned int sing_multin(uint8_t *det, uint8_t *occ_orbs, unsigned int num_elec,
                         SymmInfo *symm,
                         unsigned int (* unocc_sym_counts)[2], unsigned int num_sampl,
                         std::mt19937 &mt_obj, uint8_t (* chosen_orbs)[2], double *prob_vec);


/*! \brief Determine the number of symmetry-allowed occupied orbitals that can
 *  be chosen for single excitations from a determinant.
 *
 * Symmetry-allowed occupied orbitals are those orbitals i for which there exists
 * an unoccupied orbital a such that \Gamma_i = \Gamma_a
 *
 * \param [in] occ_orbs     list of occupied orbitals in the determinant
 *                          (length \p num_elec)
 * \param [in] num_elec     number of occupied orbitals in the determinant
 * \param [in] orb_symm     irreps of the HF orbitals in the basis
 *                          (length \p num_orb)
 * \param [in] num_orb      number of unfrozen spatial orbitals in the basis
 * \param [in] unocc_sym_counts List of unoccupied orbitals with each irrep,
 *                          as generated by count_symm_virt()
 *                          dimensions (n_irreps x 2)
 * \return number of symmetry-allowed orbitals
 */
unsigned int count_sing_allowed(uint8_t *occ_orbs, unsigned int num_elec,
                                uint8_t *orb_symm, unsigned int num_orb,
                                unsigned int (* unocc_sym_counts)[2]);


/*! \brief Count the number of symmetry-allowed single excitations given the
 * selection of an occupied orbital
 *
 * The number of symmetry-allowed single excitations is the number of virtual
 * orbitals a for which \Gamma_i = \Gamma_a, where i is the chosen occupied
 * orbital
 *
 * \param [in] occ_orbs     list of occupied orbitals in the determinant
 *                          (length \p num_elec)
 * \param [in] num_elec     number of occupied orbitals in the determinant
 * \param [in] orb_symm     irreps of the HF orbitals in the basis
 *                          (length \p num_orb)
 * \param [in] num_orb      number of unfrozen spatial orbitals in the basis
 * \param [in] unocc_sym_counts  List of unoccupied orbitals with each irrep,
 *                          as generated by count_symm_virt()
 *                          dimensions (n_irreps x 2)
 * \param [in, out] occ_choice   pointer to index of chosen symmetry-allowed occupied orbital;
 *                          upon return, contains index of orbital (without symmetry-allowed constraint)
 */
unsigned int count_sing_virt(uint8_t *occ_orbs, uint8_t num_elec,
                             uint8_t *orb_symm, uint8_t num_orb,
                             unsigned int (* unocc_sym_counts)[2],
                             uint8_t *occ_choice);


/*! \brief Calculate the weights of virtual irrep pairs corresponding to a
 *  chosen occupied pair, and the number of virtual orbital pairs in each irrep
 *  pair
 *
 * \param [in] occ_orbs     list of occupied orbitals in the origin determinant
 *                          (length \p num_elec)
 * \param [in] num_elec     number of occupied orbitals in the determinant
 * \param [in] orb_symm     irreps of the HF orbitals in the basis
 *                          (length \p num_orb)
 * \param [in] num_orb      number of unfrozen spatial orbitals in the basis
 * \param [in] unocc_sym_counts  List of unoccupied orbitals with each irrep,
 *                          as generated by count_symm_virt()
 *                          dimensions (n_irreps x 2)
 * \param [in, out] occ_choice   Pointer to index of chosen occupied pair; upon
 *                          return, \p occ_choice[0] and \p occ_choice[1]
 *                          contain the occupied orbitals
 * \param [out] virt_weights   Weights of each irrep pair
 * \param [out] virt_counts Number of choices for each irrep pair
 */
void symm_pair_wt(uint8_t *occ_orbs, unsigned int num_elec,
                  uint8_t *orb_symm, unsigned int num_orb,
                  unsigned int (* unocc_sym_counts)[2],
                  uint8_t *occ_choice,
                  double *virt_weights, uint8_t *virt_counts);


/*! \brief Calculate the ith virtual orbital with a particular spin and irrep in
 *  a determinant
 *
 * \param [in] det          bit-string representation of determinant
 * \param [in] lookup_row   row of the symmetry lookup table (from
 *                          gen_symm_lookup(), including the 0th column)
 *                          corresponding to the virtual orbital
 * \param [in] spin_shift   \p n_orb * spin of virtual orbital
 * \param [in] index        index (i) of the virtual orbital to be found
 * \return the spin orbital, or 255 if not found
 */
uint8_t virt_from_idx(uint8_t *det, uint8_t *lookup_row,
                      uint8_t spin_shift, unsigned int index);


#endif /* near_uniform_h */
