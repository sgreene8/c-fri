/*! \file
 *
 * \brief Utilities for storing and manipulating sparse vectors
 *
 * Supports sparse vectors distributed among multiple processes
 */

#ifndef vec_utils_h
#define vec_utils_h

#include <FRIES/det_hash.hpp>
#include <FRIES/Hamiltonians/hub_holstein.hpp>
#include <mpi.h>
#include <FRIES/ndarr.hpp>
#include <FRIES/compress_utils.hpp>
#include <FRIES/io_utils.hpp>
#include <vector>
#include <stack>
#include <functional>
#include <sstream>


inline void mpi_atoav(double *sendv, int *send_cts, int *disps, double *recvv, int *recv_cts) {
    MPI_Alltoallv(sendv, send_cts, disps, MPI_DOUBLE, recvv, recv_cts, disps, MPI_DOUBLE, MPI_COMM_WORLD);
}


inline void mpi_atoav(int *sendv, int *send_cts, int *disps, int *recvv, int *recv_cts) {
    MPI_Alltoallv(sendv, send_cts, disps, MPI_INT, recvv, recv_cts, disps, MPI_INT, MPI_COMM_WORLD);
}


inline void mpi_allgathv_inplace(int *arr, int *nums, int *disps) {
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, arr, nums, disps, MPI_INT, MPI_COMM_WORLD);
}


inline void mpi_allgathv_inplace(double *arr, int *nums, int *disps) {
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, arr, nums, disps, MPI_DOUBLE, MPI_COMM_WORLD);
}

template <class el_type>
class DistVec;

/*!
 * \brief Class for adding elements to a DistVec object
 * \tparam el_type Type of elements to be added to the DistVec object
 * Elements are first added to a buffer, and then the buffered elements can be distributed to the appropriate process by calling perform_add()
 */
template <class el_type>
class Adder {
public:
    /*! \brief Constructor for Adder class
     * Allocates memory for the internal buffers in the class
     * \param [in] size     Maximum number of elements per MPI process in send and receive buffers
     * \param [in] n_procs  The number of processes
     */
    Adder(size_t size, int n_procs, uint8_t n_bits) : n_bytes_(CEILING(n_bits + 1, 8)), send_idx_(n_procs, size * CEILING(n_bits + 1, 8)), send_vals_(n_procs, size), recv_idx_(n_procs, size * CEILING(n_bits + 1, 8)), recv_vals_(n_procs, size), send_cts_(n_procs), recv_cts_(n_procs), idx_disp_(n_procs), val_disp_(n_procs) {
        for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
            val_disp_[proc_idx] = proc_idx * (int)size;
            idx_disp_[proc_idx] = proc_idx * (int)size * n_bytes_;
            send_cts_[proc_idx] = 0;
        }
    }
    
    /*! \brief Remove the elements from the internal buffers and send them to the DistVec objects on their corresponding MPI processes for addition
     *
     * \param [in] parent_vec   Pointer to the DistVec object to which elements will be added
     */
    void perform_add(DistVec<el_type> *parent_vec, size_t origin);
    
    /*! \brief Remove elements from internal buffers and send them to the DistVec objects on their corresponding MPI processes for taking dot products
     *
     * \param [in] parent_vec   Pointer to the DistVec object associated with this dot product operation
     */
    void perform_dot(DistVec<el_type> *parent_vec, Matrix<el_type> &results);
    
    /*! \brief Add an element to the internal buffers
     * \param [in] idx      Index of the element to be added
     * \param [in] idx_bits     Number of bits used to encode the index
     * \param [in] val      Value of the added element
     * \param [in] ini_flag     Either 1 or 0, indicates initiator status of the added element
     * \return True if buffer is not yet full, false if it is
     */
    bool add(uint8_t *idx, uint8_t idx_bits, el_type val, int proc_idx, uint8_t ini_flag);
    bool add(uint8_t *idx, uint8_t idx_bits, el_type *vals, size_t n_vals, int proc_idx, uint8_t ini_flag);
    
    size_t size() const {
        return send_vals_.cols();
    }
    
    bool get_add_result(size_t row, size_t col, double *weight) const {
        *weight = send_vals_(row, col);
        Matrix<bool>::RowReference success((Matrix<uint8_t> *) &send_idx_, row);
        return success[col];
    }
private:
    Matrix<uint8_t> send_idx_; ///< Send buffer for element indices
    Matrix<el_type> send_vals_; ///< Send buffer for element values
    Matrix<uint8_t> recv_idx_; ///< Receive buffer for element indices
    Matrix<el_type> recv_vals_; ///< Receive buffer for element values
    std::vector<int> send_cts_; ///< Number of elements in the send buffer for each process
    std::vector<int> recv_cts_; ///< Number of elements in the receive buffer for each process
    std::vector<int> idx_disp_; ///< Displacements for MPI send/receive operations for indices
    std::vector<int> val_disp_; ///< Displacements for MPI send/receive operations for values
    uint8_t n_bytes_; ///< Number of bytes used to encode each index in the send and receive buffers
};

/*!
 * \brief Class for storing and manipulating a sparse vector
 * \tparam el_type Type of elements in the vector
 * Elements of the vector are distributed across many MPI processes, and hashing is used for efficient access.
 * Each index is uniquely associated with a particular process, as determined its hash value.
 * This class allows for multiple element values to be associated with each vector index, allowing for the storage
 * of multiple sparse vectors in a single \p DistVec object. This is useful for manipulating multiple sparse
 * vectors that are likely to have many nonzero elements in common. The values for all vectors are stored in the
 * \p values_ matrix, and \p HashTable objects handle the indexing for the elements in all vectors.
 * This class also supports much of the logic used in ran
 */
template <class el_type>
class DistVec {
private:
    Matrix<el_type> values_; ///< Values of elements for each of the sparse vectors stored in this object
    uint8_t curr_vec_idx_; ///< Current vector to consider when adding, accessing elements, etc.
    size_t n_dense_; ///< The first \p n_dense elements in the DistVec object will always be stored, even if their corresponding values are 0
    std::stack<size_t> vec_stack_; ///< Pointer to top of stack for managing available positions in the indices array
    int n_nonz_; ///< Current number of nonzero elements in vector, including all in the dense subspace
    Adder<el_type> *adder_; ///< Pointer to adder object for buffered addition of elements distributed across MPI processes
    size_t min_del_idx_; ///< Elements in \p values with indices less than this index will not be deleted when \p del_at_pos() is called
protected:
    Matrix<uint8_t> indices_; ///< Array of indices of vector elements
    size_t max_size_; ///< Maximum number of vector elements that can be stored
    size_t curr_size_; ///< Current number of vector elements stored, including intermediate zeroes
    Matrix<uint8_t> occ_orbs_; ///< Matrix containing lists of occupied orbitals for each determniant index
    uint8_t n_bits_; ///< Number of bits used to encode each index of the vector
    HashTable<ssize_t> vec_hash_; ///< Hash table for quickly finding indices in \p indices_
    HashTable<ssize_t> proc_hash_; ///< Nothing is stored in this hash table, but its hash function is used to map indices to processes
    uint64_t nonini_occ_add; ///< Number of times an addition from a noninitiator determinant to an occupied determinant occurred
    std::vector<double> matr_el_; ///< Array of pre-calculated diagonal matrix elements associated with each vector element
    std::function<double(const uint8_t *)> diag_calc_; ///< Pointer to a function used to calculate diagonal matrix elements for elements in this vector
    std::vector<bool> active_pos_; ///< Boolean vector indicating which indices are currently in the vector's hash table
    
    virtual void initialize_at_pos(size_t pos, uint8_t *orbs) {
        for (uint8_t vec_idx = 0; vec_idx < values_.rows(); vec_idx++) {
            values_(vec_idx, pos) = 0;
        }
        matr_el_[pos] = NAN;
        active_pos_[pos] = true;
        memcpy(occ_orbs_[pos], orbs, occ_orbs_.cols());
    }
    
public:

    DistVec(size_t size, Adder<el_type> *adder, uint8_t n_bits, unsigned int n_elec,
            std::function<double(const uint8_t *)> diag_fxn, uint8_t n_vecs,
            std::vector<uint32_t> rns_common, std::vector<uint32_t> rns_distinct) :
    values_(n_vecs, size),
    curr_vec_idx_(0),
    max_size_(size), curr_size_(0),
    occ_orbs_(size, n_elec),
    adder_(adder),
    n_nonz_(0),
    indices_(size, CEILING(n_bits, 8)),
    active_pos_(size),
    n_bits_(n_bits),
    n_dense_(0),
    nonini_occ_add(0),
    diag_calc_(diag_fxn),
    matr_el_(size),
    vec_hash_(size, rns_distinct),
    proc_hash_(0, rns_common),
    min_del_idx_(0) { }
    
    DistVec(size_t size, Adder<el_type> *adder, uint8_t n_bits, unsigned int n_elec,
            std::vector<uint32_t> rns_common, std::vector<uint32_t> rns_distinct) :
    DistVec(size, adder, n_bits, n_elec, nullptr, 1, rns_common, rns_distinct) {}
    
    /*! \brief Constructor for DistVec object
     * \param [in] size         Maximum number of elements to be stored in the vector
     * \param [in] add_size     Maximum number of elements per processor to use in Adder object
     * \param [in] n_bits        Number of bits used to encode each index of the vector
     * \param [in] n_elec       Number of electrons represented in each vector index
     * \param [in] n_procs      Number of MPI processes over which to distribute vector elements
     * \param [in] diag_fxn     Function used to calculate the diagonal matrix element for a vector element
     * \param [in] n_vecs       Number of vectors to store in parallel in this DistVec object
     * \param [in] rns_common   Vector of random numbers that is the same on all MPI processes
     * \param [in] rns_distinct A different vector of random numbers that need not be common to
     * all MPI processes, but should not be the same as \p rns_common
     */
    DistVec(size_t size, size_t add_size, uint8_t n_bits, unsigned int n_elec, int n_procs,
            std::function<double(const uint8_t *)> diag_fxn, uint8_t n_vecs,
            std::vector<uint32_t> rns_common, std::vector<uint32_t> rns_distinct) :
    DistVec(size, new Adder<el_type>(add_size, n_procs, n_bits),
            n_bits, n_elec, diag_fxn, n_vecs, rns_common, rns_distinct) {}
    
    DistVec(size_t size, size_t add_size, uint8_t n_bits, unsigned int n_elec, int n_procs,
            std::vector<uint32_t> rns_common, std::vector<uint32_t> rns_distinct) :
    DistVec(size, add_size, n_bits, n_elec, n_procs, nullptr, 1, rns_common, rns_distinct) {}
    
    uint8_t n_bits() {
        return n_bits_;
    }

    /*! \brief Generate list of occupied orbitals from bit-string representation of
     *  a determinant
     *
     * This implementation uses the procedure in Sec. 3.1 of Booth et al. (2014)
     * \param [in] det          bit string to be parsed
     * \param [out] occ_orbs    Occupied orbitals in the determinant
     * \return number of 1 bits in the bit string
     */
    virtual uint8_t gen_orb_list(uint8_t *det, uint8_t *occ_orbs) {
        return find_bits(det, occ_orbs, indices_.cols());
    }

    /*! \brief Calculate dot product
     *
     * Calculates dot product of the portion of a DistVec object stored on each MPI process
     * with a local sparse vector (such that the local results could be added)
     *
     * \param [in] idx2         Indices of elements in the local vector
     * \param [in] vals2         Values of elements in the local vector
     * \param [in] num2         Number of elements in the local vector
     * \param [in] hashes2      hash values of the indices of the local vector from
     *                          the hash table of vec
     * \return the value of the dot product
     */
    double dot(Matrix<uint8_t> &idx2, double *vals2, size_t num2,
               std::vector<uintmax_t> &hashes2) {
        double numer = 0;
        for (size_t hf_idx = 0; hf_idx < num2; hf_idx++) {
            ssize_t * ht_ptr = vec_hash_.read(idx2[hf_idx], hashes2[hf_idx], false);
            if (ht_ptr) {
                numer += vals2[hf_idx] * values_(curr_vec_idx_, *ht_ptr);
            }
        }
        return numer;
    }
    
    /*! \brief Same as function above, except hash values are not provided
     */
    double dot(Matrix<uint8_t> &idx2, double *vals2, size_t num2) {
        double numer = 0;
        uint8_t tmp_occ[occ_orbs_.cols()];
        for (size_t hf_idx = 0; hf_idx < num2; hf_idx++) {
            uintmax_t hash_val = idx_to_hash(idx2[hf_idx], tmp_occ);
            ssize_t * ht_ptr = vec_hash_.read(idx2[hf_idx], hash_val, false);
            if (ht_ptr) {
                numer += vals2[hf_idx] * values_(curr_vec_idx_, *ht_ptr);
            }
        }
        return numer;
    }
    
    void dot(uint8_t *idx2, double *vals2, size_t num2, Matrix<double> &results) {
        size_t n_vals = results.rows();
        size_t n_vecs = results.cols();
        uint8_t tmp_occ[occ_orbs_.cols()];
        uint8_t add_n_bytes = CEILING(n_bits_ + 1, 8);
        if (curr_vec_idx_ + n_vals > values_.rows()) {
            throw std::runtime_error("Check curr_vec_idx_ before calculating dot product");
        }
        for (size_t hf_idx = 0; hf_idx < num2; hf_idx++) {
            uint8_t *idx_ptr = &idx2[hf_idx * add_n_bytes];
            uintmax_t hash_val = idx_to_hash(idx_ptr, tmp_occ);
            ssize_t * ht_ptr = vec_hash_.read(idx_ptr, hash_val, false);
            if (ht_ptr) {
                for (size_t vec_idx = 0; vec_idx < n_vecs; vec_idx++) {
                    for (size_t dot_idx = 0; dot_idx < n_vals; dot_idx++) {
                        results(dot_idx, vec_idx) += vals2[hf_idx * n_vals + dot_idx] * values_(curr_vec_idx_ + vec_idx, *ht_ptr);
                    }
                }
            }
        }
    }
    
    /*! \brief Calculate dot product across all MPI processes
     *
     * Unlike the \p dot function above, this function calculates the overlap between vector elements on all MPI process and those in the inputted vectors
     *
     * \param [in] idx      Bit string of elements to dot with this vector
     * \param [in] occ      List of occupied orbitals in each bit string index
     * \param [in] vals     Values of elements to dot
     * \param [in] num      Number of elements to be dotted on this process
     * \return The value of the dot product
     */
    double multi_dot(Matrix<uint8_t> &idx, Matrix<uint8_t> &occ, double *vals, size_t num) {
        int n_procs = 1;
        int proc_rank = 0;
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
        size_t el_idx = 0;
        int num_added = 1;
        int proc;
        size_t max_add = adder_size();
        double tot_dprod = 0;
        while (num_added > 0) {
            num_added = 0;
            while (el_idx < num && num_added < max_add) {
                double curr_el = vals[el_idx];
                if (curr_el != 0) {
                    add(idx[el_idx], occ[el_idx], vals[el_idx], 0, &proc);
                    num_added++;
                }
                el_idx++;
            }
            num_added = sum_mpi(num_added, proc_rank, n_procs);
            tot_dprod += adder_->perform_dot(this);
        }
        return tot_dprod;
    }
    
    void perform_dot(Matrix<double> &results) {
        return adder_->perform_dot(this, results);
    }
    
    
    /*! \brief Calculate dot product of two vectors stored within the internal storage of the DistVec object
     *
     * \param [in] idx1      Row index of one of the input vectors within the \p values_ matrix
     * \param [in] idx2      Row index of one of the input vectors within the \p values_ matrix
     * \return Value of the dot product
     */
    double internal_dot(uint8_t idx1, uint8_t idx2) {
        double dprod = 0;
        if (idx1 >= values_.rows()) {
            std::stringstream error;
            error << "Error: idx1 argument to internal_dot (" << (unsigned int) idx1 << ") exceeds bounds of value matrix (" << values_.rows() << ")";
            throw std::runtime_error(error.str());
        }
        if (idx2 >= values_.rows()) {
            std::stringstream error;
            error << "Error: idx2 argument to internal_dot (" << (unsigned int) idx2 << ") exceeds bounds of value matrix (" << values_.rows() << ")";
            throw std::runtime_error(error.str());
        }
        for (size_t el_idx = 0; el_idx < curr_size_; el_idx++) {
            dprod += values_(idx1, el_idx) * values_(idx2, el_idx);
        }
        return dprod;
    }
    
    /*! \brief Double the maximum number of elements that can be stored */
    virtual void expand() {
        size_t new_max = max_size_ * 2;
        std::cout << "Increasing storage capacity in vector to " << new_max << "\n";
        indices_.reshape(new_max, indices_.cols());
        active_pos_.resize(new_max);
        matr_el_.resize(new_max);
        occ_orbs_.reshape(new_max, occ_orbs_.cols());
        
        values_.enlarge_cols(new_max, (int) curr_size_);
        max_size_ = new_max;
    }

    /*! \brief Hash function mapping vector index to MPI process
     *
     * \param [in] idx          Vector index
     * \return process index from hash value
     */
    virtual int idx_to_proc(uint8_t *idx) {
        unsigned int n_elec = (unsigned int)occ_orbs_.cols();
        uint8_t orbs[n_elec];
        gen_orb_list(idx, orbs);
        return idx_to_proc(idx, orbs);
    }
    
    /*! \brief Hash function mapping vector index to MPI process
     *
     * \param [in] idx          Vector index
     * \param [in] orbs         Array of occupied orbital indices in \p idx
     * \return process index from hash value
     */
    virtual int idx_to_proc(uint8_t *idx, uint8_t *orbs) {
        unsigned int n_elec = (unsigned int)occ_orbs_.cols();
        uintmax_t hash_val = proc_hash_.hash_fxn(orbs, n_elec, NULL, 0);
        int n_procs = 1;
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        return hash_val % n_procs;
    }

    /*! \brief Hash function mapping vector index to local hash value
     *
     * The local hash value is used to find the index on a particular processor
     *
     * \param [in] idx          Vector index
     * \param [out] orbs        Array of occupied orbital indices in \p idx
     * \return hash value
     */
    virtual uintmax_t idx_to_hash(uint8_t *idx, uint8_t *orbs) {
        unsigned int n_elec = (unsigned int)occ_orbs_.cols();
        if (gen_orb_list(idx, orbs) != n_elec) {
            uint8_t n_bytes = indices_.cols();
            char det_txt[n_bytes * 2 + 1];
            print_str(idx, n_bytes, det_txt);
            std::stringstream error;
            error << "Determinant " << det_txt << " created with an incorrect number of electrons";
            throw std::runtime_error(error.str());
        }
        return vec_hash_.hash_fxn(orbs, n_elec, NULL, 0);
    }
    
    
    /*! \brief Print number of elements in each row of vector hash table
     */
    void print_ht() {
        vec_hash_.print_ht();
    }

    /*! \brief Add an element to the DistVec object
     *
     * The element will be added to a buffer for later processing
     *
     * \param [in] idx          The index of the element in the vector
     * \param [in] val          The value of the added element
     * \param [in] ini_flag     Either 1 or 0. If 0, will only be added to a determinant that is already occupied
     * \return True if buffer is not yet full, false otherwise
     */
    bool add(uint8_t *idx, el_type val, uint8_t ini_flag) {
        if (val != 0) {
            return adder_->add(idx, n_bits_, val, idx_to_proc(idx), ini_flag);
        }
        return true;
    }
    
    bool add(uint8_t *idx, el_type *vals, size_t n_vals, uint8_t ini_flag) {
        return adder_->add(idx, n_bits_, vals, n_vals, idx_to_proc(idx), ini_flag);
    }
    
    bool add(uint8_t *idx, uint8_t *orbs, el_type val, uint8_t ini_flag) {
        return adder_->add(idx, n_bits_, val, idx_to_proc(idx, orbs), ini_flag);
    }

    /*! \brief Incorporate elements from the Adder buffer into the vector
     *
     * Elements are added if the corresponding element in \p origin is nonzero.
     * Otherwise, only elements with a nonzero initiator flag are added
     */
    void perform_add(size_t origin) {
        adder_->perform_add(this, origin);
    }
    
    /*! \brief Get the index of an unused intermediate index in the \p indices_ array, or -1 if none exists */
    ssize_t pop_stack() {
        if (vec_stack_.empty()) {
            return -1;
        }
        ssize_t ret_idx = vec_stack_.top();
        vec_stack_.pop();
        return ret_idx;
    }

    /*! \brief Delete an element from the vector
     *
     * Removes an element from the vector and modifies the hash table accordingly
     *
     * \param [in] pos          The position of the element to be deleted in \p indices_
     */
    void del_at_pos(size_t pos) {
        if (!active_pos_[pos]) {
            return;
        }
        bool all_zero = true;
        for (uint8_t vec_idx = 0; vec_idx < values_.rows(); vec_idx++) {
            if (values_(vec_idx, pos) != 0) {
                all_zero = false;
            }
        }
        if (pos >= min_del_idx_ && all_zero) {
            uint8_t *idx = indices_[pos];
            uintmax_t hash_val = idx_to_hash(idx, occ_orbs_[pos]);
            vec_stack_.push(pos);
            vec_hash_.del_entry(idx, hash_val);
            n_nonz_--;
            active_pos_[pos] = false;
        }
    }
    
    
    /*! \brief Remove zero elements from vector (if zero in all internal vectors)
     */
    void cleanup() {
        for (size_t pos = min_del_idx_; pos < curr_size_; pos++) {
            bool all_zero = true;
            for (uint8_t vec_idx = 0; vec_idx < values_.rows(); vec_idx++) {
                if (values_(vec_idx, pos) != 0) {
                    all_zero = false;
                }
            }
            if (all_zero) {
                del_at_pos(pos);
            }
        }
    }
    
    /*! \brief Set the \p min_del_idx parameter at \p curr_size_
     */
    void fix_min_del_idx() {
        min_del_idx_ = curr_size_;
    }
    
    void set_min_del_idx(size_t idx) {
        min_del_idx_ = idx;
    }
    
    /*! \returns The array used to store values in the DistVec object */
    el_type *values() const {
        return (el_type *) values_[curr_vec_idx_];
    }
    
    uint8_t num_vecs() const {
        return values_.rows();
    }
    
    Matrix<uint8_t> &indices() {
        return indices_;
    }
    
    /*! \returns The current number of elements in use in the \p indices_ and \p values arrays */
    size_t curr_size() const {
        return curr_size_;
    }
    
    size_t adder_size() const {
        return adder_->size();
    }
    
    /*! \brief Get the maximum number of elements the vector can store*/
    size_t max_size() const {
        return max_size_;
    }
    
    /*!\brief Get the current number of nonzero elements in the vector */
    int n_nonz() const {
        return n_nonz_;
    }
    
    uint64_t tot_sgn_coh() const {
        int my_rank = 0;
        int n_procs = 1;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        return sum_mpi(nonini_occ_add, my_rank, n_procs);
    }
    
    /*! \brief Add the vector at \p idx2 to the one at \p idx1
     */
    void add_vecs(uint8_t idx1, uint8_t idx2) {
        add_vecs(idx1, idx2, 1);
    }
    
    /*! \brief Add a constant times the vector at \p idx2 to the one at \p idx1
     */
    void add_vecs(uint8_t idx1, uint8_t idx2, el_type c) {
        for (size_t el_idx = 0; el_idx < curr_size_; el_idx++) {
            values_(idx1, el_idx) += values_(idx2, el_idx) * c;
        }
    }
    
    /*! \brief Copy the vector at \p src to the one at \p dst
     */
    void copy_vec(uint8_t src, uint8_t dst) {
        for (size_t el_idx = 0; el_idx < curr_size_; el_idx++) {
            values_(dst, el_idx) = values_(src, el_idx);
        }
    }
    
    /*! \brief Weight elements in the first vector v(\p idx1, i) by those in the second [1 + |v(\p idx2, i)|]^\p expo
     */
    void weight_vec(uint8_t idx1, uint8_t idx2, double expo) {
        for (size_t el_idx = 0; el_idx < curr_size_; el_idx++) {
            values_(idx1, el_idx) *= pow(1 + fabs(values_(idx2, el_idx)), expo);
        }
    }
    
    /*! \brief Zero all vector elements at \p curr_vec_idx_ without removing them from the hash table
     */
    void zero_vec() {
        std::fill(values_[curr_vec_idx_], values_[curr_vec_idx_ + 1], 0);
    }
    
    /*! \brief Change the internal vector used to store elements in this DistVec object
     *
     * \param [in] new_idx      Index of the internal vector to use
     */
    void set_curr_vec_idx(uint8_t new_idx) {
        if (new_idx < values_.rows()) {
            curr_vec_idx_ = new_idx;
        }
        else {
            std::stringstream error;
            error << "Argument to set_curr_vec_idx (" << (unsigned int) new_idx << ") is out of bounds";
            throw std::runtime_error(error.str());
        }
    }
    
    uint8_t curr_vec_idx() const {
        return curr_vec_idx_;
    }
    
    /*! \brief Add elements destined for this process to the DistVec object
     * \param [in] indices Indices of the elements to be added
     * \param [in, out] vals     Values of the elements to be added. Upon return, contains 2nd-order perturbative
     *                      correction (see Ghanem et al., JCP 151, 224108, 2019)
     * \param [in] count    Number of elements to be added
     */
    void add_elements(uint8_t *indices, el_type *vals, size_t count, size_t origin) {
        uint8_t add_n_bytes = CEILING(n_bits_ + 1, 8);
        uint8_t vec_n_bytes = indices_.cols();
        uint8_t tmp_occ[occ_orbs_.cols()];
        for (size_t el_idx = 0; el_idx < count; el_idx++) {
            uint8_t *new_idx = &indices[el_idx * add_n_bytes];
            int ini_flag = read_bit(new_idx, n_bits_);
            if (ini_flag) {
                zero_bit(new_idx, n_bits_);
            }
            uintmax_t hash_val = idx_to_hash(new_idx, tmp_occ);
            ssize_t *idx_ptr = vec_hash_.read(new_idx, hash_val, ini_flag);
            if (idx_ptr && *idx_ptr == -1) {
                *idx_ptr = pop_stack();
                if (*idx_ptr == -1) {
                    if (curr_size_ >= max_size_) {
                        expand();
                    }
                    *idx_ptr = curr_size_;
                    curr_size_++;
                }
                memcpy(indices_[*idx_ptr], new_idx, vec_n_bytes);
                initialize_at_pos(*idx_ptr, tmp_occ);
                n_nonz_++;
            }
            if (idx_ptr) {
                bool nonz = values_(origin, *idx_ptr) != 0;
                bool should_add = ini_flag || nonz;
                nonini_occ_add += !ini_flag && nonz;
                if (should_add) {
                    values_(curr_vec_idx_, *idx_ptr) += vals[el_idx];
                }
                vals[el_idx] = 0;
            }
        }
    }
    
    /*! \brief Get a pointer to a value in the \p values_ matrix of the DistVec object
     
     * \param [in] pos          The position of the corresponding index in the \p indices_ array
     */
    el_type *operator[](size_t pos) {
        return &values_(curr_vec_idx_, pos);
    }
    
    el_type *operator()(size_t vec_idx, size_t pos) {
        return &values_(vec_idx, pos);
    }

    /*! \brief Get a pointer to the list of occupied orbitals corresponding to an
     * existing determinant index in the vector
     *
     * \param [in] pos          The row index of the index in the \p indices_  matrix
     */
    uint8_t *orbs_at_pos(size_t pos) {
        return occ_orbs_[pos];
    }
    
    Matrix<uint8_t> &occ_orbs() {
        return occ_orbs_;
    }
    
    /*! \brief Get a pointer to the diagonal matrix element corresponding to an element in the DistVec object
     
     * \param [in] pos          The position of the corresponding index in the \p indices_ array
     */
    double matr_el_at_pos(size_t pos) {
        if (std::isnan(matr_el_[pos])) {
            matr_el_[pos] = diag_calc_(occ_orbs_[pos]);
        }
        return matr_el_[pos];
    }

    /*! \brief Calculate the sum of the magnitudes of the vector elements on each MPI process
     *
     * \return The sum of the magnitudes on this process
     */
    double local_norm() const {
        double norm = 0;
        for (size_t idx = 0; idx < curr_size_; idx++) {
            norm += fabs(values_(curr_vec_idx_, idx));
        }
        return norm;
    }
    
    /*! \brief Calculate the sum of the squares of the vector elements on each MPI process
     *
     * \return The two-norm of the elements on this process
     */
    double two_norm() const {
        double norm = 0;
        for (size_t idx = 0; idx < curr_size_; idx++) {
            norm += values_(curr_vec_idx_, idx) * values_(curr_vec_idx_, idx);
        }
        return norm;
    }

    /*! \brief Save a DistVec object to disk in binary format
     *
     * The vector indices from each MPI process are stored in the file
     * [path]dets[MPI rank].dat, and the values at [path]vals[MPI rank].dat
     * Only the values from \p start_idx to \p start_idx + \p n_vecs - 1 are saved
     *
     * \param [in] path         Location where the files are to be stored
     * \param [in] start_idx    Index of the first vector to write to disk
     * \param [in] n_vecs       Number of vectors to write to disk
     */
    void save(const std::string &path, uint8_t start_idx, uint8_t n_vecs)  {
        int my_rank = 0;
        int n_procs = 1;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        
        size_t el_size = sizeof(el_type);
        
        std::stringstream buffer;
        buffer << path << "dets" << my_rank << ".dat";
        std::ofstream file_p(buffer.str(), std::ios::binary);
        file_p.write((const char *)indices_.data(), curr_size_ * indices_.cols());
        file_p.close();
        
        buffer.str("");
        buffer << path << "vals" << my_rank << ".dat";
        file_p.open(buffer.str(), std::ios::binary);
        for (uint8_t vec_idx = 0; vec_idx < n_vecs; vec_idx++) {
            file_p.write((const char *)values_[start_idx + vec_idx], el_size * curr_size_);
        }
        file_p.close();
        
        int dense_sizes[n_procs];
        MPI_Gather(&n_dense_, 1, MPI_INT, dense_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (my_rank == 0) {
            buffer.str("");
            buffer << path << "dense.txt";
            file_p.open(buffer.str());
            for (int proc_idx = 0; proc_idx < n_procs - 1; proc_idx++) {
                file_p << dense_sizes[proc_idx] << ",";
            }
            file_p << dense_sizes[n_procs - 1] << '\n';
        }
    }
    
    void save(const std::string &path) {
        save(path, 0, values_.rows());
    }

    /*! \brief Load a vector from disk in binary format
     *
     * The vector indices from each MPI process are read from the file
     * [path]dets[MPI rank].dat, and the values from [path]vals[MPI rank].dat
     *
     * \param [in] path         Location from which to read the files
     * \param [in] n_vecs     Number of vectors to load
     * \return Size of the dense subspace
     */
    size_t load(const std::string &path, uint8_t n_vecs) {
        if (curr_vec_idx_ + n_vecs > values_.rows()) {
            std::stringstream msg;
            msg << "Number of vectors requested in load function (" << (int) n_vecs << ") will put you over the capacity of this DistVec object (" << (int) values_.rows() << "), given the value of instance variable curr_vec_idx_ (" << (int) curr_vec_idx_ << ")";
            throw std::runtime_error(msg.str());
        }
        int my_rank = 0;
        int n_procs = 1;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        
        std::stringstream buffer;
        int dense_sizes[n_procs];
        if (my_rank == 0) {
            buffer << path << "dense.txt";
            read_csv(dense_sizes, buffer.str());
        }
        MPI_Scatter(dense_sizes, 1, MPI_INT, &n_dense_, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        size_t el_size = sizeof(el_type);
        
        size_t n_dets;
        size_t n_bytes = indices_.cols();
        buffer.str("");
        buffer << path << "dets" << my_rank << ".dat";
        std::ifstream file_p(buffer.str(), std::ios::binary | std::ios::ate);
        if (!file_p.is_open()) {
            std::stringstream msg;
            msg << "Could not open saved binary vector file at path " << buffer.str();
            throw std::runtime_error(msg.str());
        }
        auto f_size = file_p.tellg();
        n_dets = f_size / n_bytes;
        while (n_dets > max_size_) {
            expand();
        }
        file_p.seekg(0, std::ios::beg);
        file_p.read((char *)indices_.data(), n_dets * n_bytes);
        file_p.close();
        
        buffer.str("");
        buffer << path << "vals" << my_rank << ".dat";
        file_p.open(buffer.str(), std::ios::binary);
        if (!file_p.is_open()) {
            std::stringstream msg;
            msg << "Could not open saved binary vector file at path " << buffer.str();
            throw std::runtime_error(msg.str());
        }
        for (uint8_t vec_idx = 0; vec_idx < n_vecs; vec_idx++) {
            file_p.read((char *)values_[curr_vec_idx_ + vec_idx], el_size * n_dets);
        }
        file_p.close();
        
        n_nonz_ = 0;
        uint8_t tmp_orbs[occ_orbs_.cols()];
        double tmp_vals[n_vecs];
        for (size_t det_idx = 0; det_idx < n_dets; det_idx++) {
            bool is_nonz = det_idx < n_dense_ || det_idx < min_del_idx_;
            if (!is_nonz) {
                for (uint8_t vec_idx = 0; vec_idx < n_vecs; vec_idx++) {
                    if (fabs(values_(curr_vec_idx_ + vec_idx, det_idx)) > 1e-9) {
                        is_nonz = true;
                    }
                }
            }
            if (is_nonz) {
                uint8_t *new_idx = indices_[det_idx];
                uintmax_t hash_val = idx_to_hash(new_idx, tmp_orbs);
                ssize_t *idx_ptr = vec_hash_.read(new_idx, hash_val, true);
                *idx_ptr = n_nonz_;
                memmove(indices_[n_nonz_], new_idx, n_bytes);
                for (size_t val_idx = 0; val_idx < n_vecs; val_idx++) {
                    tmp_vals[val_idx] = values_(curr_vec_idx_ + val_idx, det_idx);
                }
                initialize_at_pos(n_nonz_, tmp_orbs);
                for (uint8_t vec_idx = 0; vec_idx < n_vecs; vec_idx++) {
                    values_(curr_vec_idx_ + vec_idx, n_nonz_) = tmp_vals[vec_idx];
                }
                n_nonz_++;
            }
        }
        curr_size_ = n_nonz_;
        return n_dense_;
    }
    
    size_t load(const std::string &path) {
        return load(path, values_.rows());
    }
    
    /*! \brief Load all of the vector indices defining the dense subspace from disk, and initialize the corresponding vector elements to 0
     *
     * Indices must be stored on disk as ≤64-bit integers
     *
     * \param [in] read_path     Path to the file where the indices are stored
     * \param [in] save_dir      Directory in which to store a file containing the length of the dense subspace on each MPI process
     * \return Size of the dense subspace
     */
    size_t init_dense(const std::string &read_path, const std::string &save_dir) {
        int n_procs = 1;
        int my_rank = 0;
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        
        size_t n_loaded = 0;
        if (my_rank == 0) {
            n_loaded = read_dets(read_path, indices_);
        }
        for (size_t idx = 0; idx < n_loaded; idx++) {
            add(indices_[idx], 1, 1);
        }
        perform_add(0);
        
        n_dense_ = curr_size_;
        for (uint8_t vec_idx = 0; vec_idx < values_.rows(); vec_idx++) {
            bzero(values_[vec_idx], n_dense_ * sizeof(el_type));
        }

        int dense_sizes[n_procs];
        dense_sizes[my_rank] = (int) n_dense_;
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, dense_sizes, 1, MPI_INT, MPI_COMM_WORLD);
        if (my_rank == 0) {
            std::stringstream buf;
            buf << save_dir << "dense.txt";
            std::ofstream dense_f(buf.str());
            if (!dense_f.is_open()) {
                std::stringstream msg;
                msg << "Could not load deterministic subspace from file at path " << buf.str();
                throw std::runtime_error(msg.str());
            }
            for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
                dense_f << dense_sizes[proc_idx] << ",";
            }
            dense_f << "\n";
            dense_f.close();
        }
        return n_dense_;
    }
    
    
    /*! \brief Calculate the one-norm of the vector in the dense subspace
     * \return The total norm from all processes
     */
    el_type dense_norm() {
        el_type result = 0;
        el_type element;
        for (size_t det_idx = 0; det_idx < n_dense_; det_idx++) {
            element = values_(curr_vec_idx_, det_idx);
            result += element >= 0 ? element : -element;
        }
        int n_procs = 1;
        int my_rank = 0;
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        el_type glob_norm;
        glob_norm = sum_mpi(result, my_rank, n_procs);
        return glob_norm;
    }
    
    /*! \brief Collect all of the vector elements from other MPI processes and accumulate them in the vector on each process */
    void collect_procs() {
        int n_procs = 1;
        int my_rank = 0;
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        int vec_sizes[n_procs];
        int idx_sizes[n_procs];
        int n_bytes = (int) indices_.cols();
        vec_sizes[my_rank] = (int)curr_size_;
        idx_sizes[my_rank] = (int)curr_size_ * n_bytes;
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, vec_sizes, 1, MPI_INT, MPI_COMM_WORLD);
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, idx_sizes, 1, MPI_INT, MPI_COMM_WORLD);
        int tot_size = 0;
        int disps[n_procs];
        int idx_disps[n_procs];
        for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
            idx_disps[proc_idx] = tot_size * n_bytes;
            disps[proc_idx] = tot_size;
            tot_size += vec_sizes[proc_idx];
        }
        size_t el_size = sizeof(el_type);
        if (tot_size > max_size_) {
            indices_.reshape(tot_size, n_bytes);
            values_.enlarge_cols(tot_size, (int) curr_size_);
        }
        memmove(indices_[disps[my_rank]], indices_.data(), vec_sizes[my_rank] * n_bytes);
        for (uint8_t vec_idx = 0; vec_idx < values_.rows(); vec_idx++) {
            memmove(&values_(vec_idx, disps[my_rank]), values_[vec_idx], vec_sizes[my_rank] * el_size);
            mpi_allgathv_inplace(values_[vec_idx], vec_sizes, disps);
        }
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, indices_.data(), idx_sizes, idx_disps, MPI_UINT8_T, MPI_COMM_WORLD);
        curr_size_ = tot_size;
    }
};


template <class el_type>
bool Adder<el_type>::add(uint8_t *idx, uint8_t idx_bits, el_type val, int proc_idx, uint8_t ini_flag) {
    int *count = &send_cts_[proc_idx];
    if (*count >= send_vals_.cols()) {
        throw std::runtime_error("Too many elements added to Adder - must call perform_add() more frequently.");
    }
    uint8_t *cpy_idx = &send_idx_[proc_idx][*count * n_bytes_];
    cpy_idx[n_bytes_ - 1] = 0; // To prevent buffer overflow in hash function after elements are added
    memcpy(cpy_idx, idx, CEILING(idx_bits, 8));
    if (ini_flag) {
        set_bit(cpy_idx, idx_bits);
    }
    send_vals_(proc_idx, *count) = val;
    (*count)++;
    return (*count) < send_vals_.cols();
}

template <class el_type>
bool Adder<el_type>::add(uint8_t *idx, uint8_t idx_bits, el_type *vals, size_t n_vals, int proc_idx, uint8_t ini_flag) {
    int *count = &send_cts_[proc_idx];
    if (*count * n_vals >= send_vals_.cols()) {
        throw std::runtime_error("Too many elements added to Adder in the process of taking a dot product - must call perform_dot() more frequently.");
    }
    uint8_t *cpy_idx = &send_idx_[proc_idx][*count * n_bytes_];
    cpy_idx[n_bytes_ - 1] = 0; // To prevent buffer overflow in hash function after elements are added
    std::copy(idx, idx + CEILING(idx_bits, 8), cpy_idx);
    if (ini_flag) {
        set_bit(cpy_idx, idx_bits);
    }
    std::copy(vals, vals + n_vals, &send_vals_(proc_idx, *count * n_vals));
    (*count)++;
    return (*count) < send_vals_.cols();
}

template <class el_type>
void Adder<el_type>::perform_add(DistVec<el_type> *parent_vec, size_t origin) {
    int n_procs = 1;
    
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Alltoall(send_cts_.data(), 1, MPI_INT, recv_cts_.data(), 1, MPI_INT, MPI_COMM_WORLD);
    
    int send_idx_cts[n_procs];
    int recv_idx_cts[n_procs];
    for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        send_idx_cts[proc_idx] = send_cts_[proc_idx] * n_bytes_;
        recv_idx_cts[proc_idx] = recv_cts_[proc_idx] * n_bytes_;
    }
    
    MPI_Alltoallv(send_idx_.data(), send_idx_cts, idx_disp_.data(), MPI_UINT8_T, recv_idx_.data(), recv_idx_cts, idx_disp_.data(), MPI_UINT8_T, MPI_COMM_WORLD);
    mpi_atoav(send_vals_.data(), send_cts_.data(), val_disp_.data(), recv_vals_.data(), recv_cts_.data());
    // Move elements from receiving buffers to vector
    for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        parent_vec->add_elements(recv_idx_[proc_idx], recv_vals_[proc_idx], recv_cts_[proc_idx], origin);
    }
    mpi_atoav(recv_vals_.data(), recv_cts_.data(), val_disp_.data(), send_vals_.data(), send_cts_.data());
    for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        recv_idx_cts[proc_idx] = CEILING(recv_cts_[proc_idx], 8);
        send_idx_cts[proc_idx] = CEILING(send_cts_[proc_idx], 8);
    }
    MPI_Alltoallv(recv_idx_.data(), recv_idx_cts, idx_disp_.data(), MPI_UINT8_T, send_idx_.data(), send_idx_cts, idx_disp_.data(), MPI_UINT8_T, MPI_COMM_WORLD);
    for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        send_cts_[proc_idx] = 0;
    }
}


template <class el_type>
void Adder<el_type>::perform_dot(DistVec<el_type> *parent_vec, Matrix<el_type> &results) {
    int n_procs = 1;
    int my_rank = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Alltoall(send_cts_.data(), 1, MPI_INT, recv_cts_.data(), 1, MPI_INT, MPI_COMM_WORLD);
    
    int send_idx_cts[n_procs];
    int recv_idx_cts[n_procs];
    for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        send_idx_cts[proc_idx] = send_cts_[proc_idx] * n_bytes_;
        recv_idx_cts[proc_idx] = recv_cts_[proc_idx] * n_bytes_;
    }
    
    MPI_Alltoallv(send_idx_.data(), send_idx_cts, idx_disp_.data(), MPI_UINT8_T, recv_idx_.data(), recv_idx_cts, idx_disp_.data(), MPI_UINT8_T, MPI_COMM_WORLD);
    size_t n_vals = results.rows();
    for (size_t proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        send_cts_[proc_idx] *= n_vals;
        recv_cts_[proc_idx] *= n_vals;
    }
    mpi_atoav(send_vals_.data(), send_cts_.data(), val_disp_.data(), recv_vals_.data(), recv_cts_.data());
    for (int proc_idx = 0; proc_idx < n_procs; proc_idx++) {
        parent_vec->dot(recv_idx_[proc_idx], recv_vals_[proc_idx], recv_cts_[proc_idx] / n_vals, results);
        send_cts_[proc_idx] = 0;
    }
}


void compress_vecs(DistVec<double> &vectors, size_t start_idx, size_t end_idx, unsigned int compress_size,
                       std::vector<size_t> &srt_scratch, std::vector<bool> &keep_scratch,
                       std::vector<bool> &del_arr, std::mt19937 &rn_gen);

void compress_vecs_sys(DistVec<double> &vectors, size_t start_idx, size_t end_idx, unsigned int compress_size,
                       std::vector<size_t> &srt_scratch, std::vector<bool> &keep_scratch,
                       std::vector<bool> &del_arr, std::mt19937 &rn_gen);

void compress_vecs_multi(DistVec<double> &vectors, size_t start_idx, size_t end_idx, unsigned int compress_size,
                         std::vector<size_t> &srt_scratch, std::vector<bool> &keep_scratch,
                         std::vector<bool> &del_arr, std::mt19937 &rn_gen);

#endif /* vec_utils_h */
