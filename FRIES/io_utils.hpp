/*! \file
 *
 * \brief Utilities for reading/writing data from/to disk.
 */

#ifndef io_utils_h
#define io_utils_h

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <FRIES/Ext_Libs/csvparser.h>
#include <FRIES/mpi_switch.h>
#include <FRIES/vec_utils.hpp>
#include <FRIES/ndarr.hpp>

/*! \brief Read an array of floating-point numbers from a .csv file
 *
 * \param [out] buf     Array in which read-in numbers are stored
 * \param [in] fname    Path of file
 * \returns          Total number of values read from the file
 */
size_t read_csv(double *buf, char *fname);

/*! \brief Read an array of unsigned integers from a .csv file
 *
 * \param [out] buf     Array in which read-in numbers are stored
 * \param [in] fname    Path of file
 * \returns          Total number of values read from the file
 */
size_t read_csv(int *buf, char *fname);


/*! \brief Read an array of unsigned bytes from a .csv file
 *
 * \param [out] buf     Array in which read-in numbers are stored
 * \param [in] fname    Path of file
 * \returns          Total number of values read from the file 
 */
size_t read_csv(uint8_t *buf, char *fname);


/*! \brief Data structure containing the output of a Hartree-Fock calculation */
struct hf_input {
    unsigned int n_elec; ///< Total number of electrons (including frozen) in the system
    unsigned int n_frz; ///< Suggested number of core electrons to freeze
    unsigned int n_orb; ///< Number of spatial orbitals in the HF basis
    FourDArr *eris; ///< Pointer to 4-D array of 2-electron integrals
    Matrix<double> *hcore; ///< Pointer to matrix of 1-electron integrals
    double hf_en; ///< HF electronic energy
    double eps; ///< Suggested imaginary time step to use in DMC calculations
    uint8_t *symm; ///< Irreps of orbitals in the HF basis
};


/*! \brief Data structure describing the parameters of a Hubbard-Holstein
 * calculation
 */
struct hh_input {
    unsigned int n_elec; ///< Total number of electrons in the system
    unsigned int lat_len; ///< Number of sites along one dimension of the lattice
    unsigned int n_dim; ///< Dimensionality of the lattice
    double elec_int; ///< On-site repulsion term
    double eps; ///< Suggested imaginary time step to use in DMC calculations
    double hf_en; ///< HF electronic energy
    double elec_ph; ///< Electron-phonon coupling
    double ph_freq; ///< Phonon energy
};


/*! \brief Read in parameters from a Hartree-Fock calculation:
 * total number of electrons
 * number of (unfrozen) orbitals
 * number of frozen (core) electrons
 * imaginary time step (eps)
 * HF electronic energy
 * matrix of one-electron integrals
 * tensor of two-electron integrals
 *
 * \param [in] hf_dir       Path to directory where the files sys_params.txt,
 *                          eris.txt, hcore.txt, and symm.txt are stored
 * \param [in] in_struct    Structure where the data will be stored
 * \return 0 if successful, -1 if unsuccessful
 */
int parse_hf_input(const char *hf_dir, hf_input *in_struct);


/*! \brief Read in the following parameters for a Hubbard-Holstein calculation
 * total number of electrons
 * number of sites along one dimension of the lattice
 * dimensionality of the lattice
 * imaginary time step (eps)
 * electron-electron interaction term, U
 * phonon energy, omega
 * electron-phonon coupling, lambda
 * HF electronic energy
 *
 * \param [in] hh_path      Path to the file containing the Hubbard-Holstein
 *                          parameters
 * \param [in] in_struct    Structure where the data will be stored
 * \return 0 if successful, -1 if unsuccessful
 */
int parse_hh_input(const char *hh_path, hh_input *in_struct);


/*! \brief Load a sparse vector in .txt format from disk
 *
 * The vector is loaded only onto the 0th MPI process
 *
 * \param [in] prefix       prefix of files containing the vector, including the
 *                          directory. File names should be in the format
 *                          [prefix]dets and [prefix]vals
 * \param [out] dets        Matrix in which to store the element indices read in. Currently supports only reading in integers less than 64 bits
 * \param [out] vals        Array of element values read in
 * \param [in] type         Data type of the vector
 * \return total number of elements read in
 */
size_t load_vec_txt(const char *prefix, Matrix<uint8_t> &dets, void *vals, dtype type);


/*! \brief Read an array of determinants from disk
 
* Determinants are loaded only onto the 0th MPI process
* Determinants must be stored as ≤64-bit integers
 *
 * \param [in] path     Path to the file where the determinants are stored
 * \param [out] dets        Matrix in which to store the element indices read in. Currently supports only reading in integers less than 64 bits
 * \return total number of elements read in
 */
size_t read_dets(const char *path, Matrix<uint8_t> &dets);


/*! \brief Save to disk the array of random numbers used to assign Slater
 * determinants to MPI processes
 *
 * This function only does something on the 0th MPI process. The numbers are
 * saved in binary format
 *
 * \param [in] path         File will be saved at the path [path]hash.dat
 * \param [in] proc_hash    Array of random numbers to save (length \p n_hash)
 */
void save_proc_hash(const char *path, unsigned int *proc_hash, size_t n_hash);


/*! \brief Load from disk the array of random numbers used to assign Slater
 * determinants to MPI processes
 *
 * The random numbers are loaded onto all MPI processes
 *
 * \param [in] path         File will be loaded from the path [path]hash.dat
 * \param [out] proc_hash   Numbers loaded from disk
 */
void load_proc_hash(const char *path, unsigned int *proc_hash);


/*! \brief Load from disk the diagonal elements of the 1-RDM
 *
 * \param [in] path     Path of the file from which to load
 * \param [out] vals        Values loaded from the file
 */
void load_rdm(const char *path, double *vals);


#endif /* io_utils_h */
