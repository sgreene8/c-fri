/*! \file
 *
 * \brief Utilities for storing and manipulating sparse vectors
 *
 * Supports sparse vectors distributed among multiple processes if USE_MPI is
 * defined
 */

#ifndef vec_utils_h
#define vec_utils_h

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <FRIES/det_store.h>
#include <FRIES/Hamiltonians/hub_holstein.h>
#include <FRIES/Ext_Libs/dcmt/dc.h>
#include <FRIES/mpi_switch.h>
#include <FRIES/ndarr.hpp>
#include <vector>

//#ifdef __cplusplus
//extern "C" {
//#endif

using namespace std;

template <class el_type>
class DistVec;

/*! \brief Struct used to add elements to sparse vector
 *
 * Elements to be added are buffered in send buffers until perform_add() is
 * called, at which point they are distributed into receive buffers located in
 * their corresponding MPI processes
 */
//typedef struct{
//    dtype type; ///< Type of elements to be added
//    size_t size; ///< Maximum number of elements per MPI process in send and receive buffers
//    long long *send_idx; ///< Send buffer for element indices
//    long long *recv_idx; ///< Receive buffer for element indices
//    void *send_vals; ///< Send buffer for element values
//    void *recv_vals; ///< Receive buffer for element values
//    Matrix<long long> *send_idx;
//    Matrix<long long> *recv_idx;
//    int *send_cts; ///< Number of elements in the send buffer for each process
//    int *recv_cts; ///< Number of elements in the receive buffer for each process
//    int *displacements; ///< Array positions in buffers corresponding to each process
//} adder;
template <class el_type>
class Adder {
public:
    Adder(size_t size, int n_procs, dtype type, DistVec<el_type> *vec) : send_idx_(size, n_procs), send_vals_(size, n_procs), recv_idx_(size, n_procs), recv_vals_(size, n_procs), type_(type), parent_vec_(vec) {
        send_cts_ = (int *)malloc(sizeof(int) * n_procs);
        recv_cts_ = (int *) malloc(sizeof(int) * n_procs);
        displacements_ = (int *) malloc(sizeof(int) * n_procs);
        int proc_idx;
        for (proc_idx = 0; proc_idx < n_procs; proc_idx++) {
            displacements_[proc_idx] = proc_idx * (int)size;
            send_cts_[proc_idx] = 0;
        }
    }
    ~Adder() {
        free(send_cts_);
        free(recv_cts_);
        free(displacements_);
    }
    Adder(const Adder &a) = delete;
    Adder& operator= (const Adder &a) = delete;
    
    void perform_add(long long ini_bit);
    void add(long long idx, el_type val, long long ini_flag);
private:
    Matrix<long long> send_idx_;
    Matrix<el_type> send_vals_;
    Matrix<long long> recv_idx_;
    Matrix<el_type> recv_vals_;
    int *send_cts_;
    int *recv_cts_;
    int *displacements_;
    DistVec<el_type> *parent_vec_;
    dtype type_;
    
    void enlarge_() {
        printf("Increasing storage capacity in adder\n");
        size_t new_size = send_idx_.rows() * 2;
        send_idx_.enlarge(new_size);
        send_vals_.enlarge(new_size);
        recv_idx_.enlarge(new_size);
        recv_vals_.enlarge(new_size);
    }
};


/*! \brief Setup an adder struct
 *
 * \param [in] size         Maximum number of elements per MPI process to use in
 *                          buffers
 * \param [in] type         Type of elements to be added
 * \return pointer to newly created adder struct
 */
//adder *init_adder(size_t size, dtype type);

template <class el_type>
class DistVec {
    long long *indices_;
    std::vector<el_type> values_;
    double *matr_el_;
    size_t max_size_;
    size_t curr_size_;
    hash_table *vec_hash_;
    stack_entry *vec_stack_;
    byte_table *tabl_;
    unsigned int n_elec_;
    Matrix<unsigned char> occ_orbs_;
    Matrix<unsigned char> neighb_;
    unsigned int n_sites_;
    Adder<el_type> adder_;
    int n_nonz_;
    dtype type_;
public:
    unsigned int *proc_scrambler_;
    DistVec(size_t size, size_t add_size, mt_struct *rn_ptr, unsigned int n_orb,
            unsigned int n_elec, int n_sites, int n_procs, dtype type) : n_elec_(n_elec), n_sites_(n_sites), values_(size), max_size_(size), curr_size_(0), vec_stack_(NULL), occ_orbs_(size, n_elec), adder_(add_size, n_procs, type, this), n_nonz_(0), type_(type), neighb_(n_sites ? size : 0, 2 * (n_elec + 1)) {
        indices_ = (long long *) malloc(sizeof(long long) * size);
        matr_el_ = (double *)malloc(sizeof(double) * size);
        vec_hash_ = setup_ht(size, rn_ptr, 2 * n_orb);
        tabl_ = gen_byte_table();
    }
    ~DistVec() {
        free(indices_);
        free(matr_el_);
    }
    DistVec(const DistVec &d) = delete;
    DistVec& operator= (const DistVec& d) = delete;
    
    double dot(long long *idx2, double *vals2, size_t num2,
               unsigned long long *hashes2) {
        size_t hf_idx;
        ssize_t *ht_ptr;
        double numer = 0;
        for (hf_idx = 0; hf_idx < num2; hf_idx++) {
            ht_ptr = read_ht(vec_hash_, idx2[hf_idx], hashes2[hf_idx], 0);
            if (ht_ptr) {
                numer += vals2[hf_idx] * values_[*ht_ptr];
            }
        }
        return numer;
    }
    
    void expand() {
        printf("Increasing storage capacity in vector\n");
        size_t new_max = max_size_ * 2;
        indices_ = (long long *)realloc(indices_, sizeof(long long) * new_max);
        matr_el_ = (double *)realloc(matr_el_, sizeof(double) * new_max);
        occ_orbs_.enlarge(new_max);
        if (n_sites_) {
            neighb_.enlarge(new_max * 2);
        }
        values_.resize(new_max);
    }
    
    int idx_to_proc(long long idx) {
        unsigned long long hash_val = idx_to_hash(idx);
        int n_procs = 1;
#ifdef USE_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
#endif
        return hash_val % n_procs;
    }
    
    unsigned long long idx_to_hash(long long idx) {
        unsigned char orbs[n_elec_];
        gen_orb_list(idx, tabl_, orbs);
        return hash_fxn(orbs, n_elec_, vec_hash_->scrambler);
    }
    
    void add(long long idx, el_type val, long long ini_flag) {
        if (val != 0) {
            adder_.add(idx, val, ini_flag);
        }
    }
    
    void perform_add(long long ini_bit) {
        adder_.perform_add(ini_bit);
    }
    
    ssize_t pop_stack() {
        stack_entry *head = vec_stack_;
        if (!head) {
            return -1;
        }
        ssize_t ret_idx = head->idx;
        vec_stack_ = head->next;
        free(head);
        return ret_idx;
    }
    
    void push_stack(size_t idx) {
        stack_entry *new_entry = (stack_entry*) malloc(sizeof(stack_entry));
        new_entry->idx = idx;
        new_entry->next = vec_stack_;
        vec_stack_ = new_entry;
    }
    
    void del_at_pos(size_t pos) {
        long long idx = indices_[pos];
        unsigned long long hash_val = idx_to_hash(idx);
        push_stack(pos);
        del_ht(vec_hash_, idx, hash_val);
        n_nonz_--;
    }
    
    long long *indices() const {
        return indices_;
    }
    void *values() const {
        return (void *)values_.data();
    }
    
    size_t curr_size() const {
        return curr_size_;
    }
    
    int n_nonz() const {
        return n_nonz_;
    }
    
    byte_table *tabl() const {
        return tabl_;
    }
    
    const Matrix<unsigned char> &neighb() const{
        return neighb_;
    }
    
    dtype type() const {
        return type_;
    }
    
    void add_elements(long long *indices, el_type *vals, size_t count, long long ini_bit) {
        size_t el_idx;
        for (el_idx = 0; el_idx < count; el_idx++) {
            long long new_idx = indices[el_idx];
            int ini_flag = !(!(new_idx & ini_bit));
            new_idx &= ini_bit - 1;
            unsigned long long hash_val = idx_to_hash(new_idx);
            ssize_t *idx_ptr = read_ht(vec_hash_, new_idx, hash_val, ini_flag);
            if (idx_ptr && *idx_ptr == -1) {
                *idx_ptr = pop_stack();
                if (*idx_ptr == -1) {
                    if (curr_size_ >= max_size_) {
                        expand();
                    }
                    *idx_ptr = curr_size_;
                    curr_size_++;
                }
                values_[*idx_ptr] = 0;
                if (gen_orb_list(new_idx, tabl_, occ_orbs_[*idx_ptr]) != n_elec_) {
                    fprintf(stderr, "Error: determinant %lld created with an incorrect number of electrons.\n", new_idx);
                }
                indices_[*idx_ptr] = new_idx;
                matr_el_[*idx_ptr] = NAN;
                n_nonz_++;
                if (n_sites_) {
                    find_neighbors_1D(new_idx, n_sites_, tabl_, n_elec_, neighb_[*idx_ptr]);
                }
            }
            int del_bool = 0;
            if (ini_flag || (idx_ptr && (values_[*idx_ptr] * vals[el_idx]) > 0)) {
                values_[*idx_ptr] += vals[el_idx];
                del_bool = values_[*idx_ptr] == 0;
            }
            if (del_bool == 1) {
                push_stack(*idx_ptr);
                del_ht(vec_hash_, new_idx, hash_val);
                n_nonz_--;
            }
        }
    }
    el_type *operator[](size_t pos) {
        return &values_[pos];
    }
    
    unsigned char *orbs_at_pos(size_t pos) {
        return occ_orbs_[pos];
    }
    
    double *matr_el_at_pos(size_t idx) {
        return &matr_el_[idx];
    }
    
    double local_norm() {
        double norm = 0;
        size_t idx;
        for (idx = 0; idx < curr_size_; idx++) {
            norm += fabs(values_[idx]);
        }
        return norm;
    }
    
    void save(const char *path)  {
        int my_rank = 0;
    #ifdef USE_MPI
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    #endif
        
        size_t el_size = 0;
        if (type_ == INT) {
            el_size = sizeof(int);
        }
        else if (type_ == DOUB) {
            el_size = sizeof(double);
        }
        
        char buffer[100];
        sprintf(buffer, "%sdets%d.dat", path, my_rank);
        FILE *file_p = fopen(buffer, "wb");
        fwrite(indices_, sizeof(long long), curr_size_, file_p);
        fclose(file_p);
        
        sprintf(buffer, "%svals%d.dat", path, my_rank);
        file_p = fopen(buffer, "wb");
        fwrite(values_.data(), el_size, curr_size_, file_p);
        fclose(file_p);
    }
    
    void load(const char *path) {
        int my_rank = 0;
    #ifdef USE_MPI
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    #endif
        
        size_t el_size = 0;
        if (type_ == INT) {
            el_size = sizeof(int);
        }
        else if (type_ == DOUB) {
            el_size = sizeof(double);
        }
        
        size_t n_dets;
        char buffer[100];
        sprintf(buffer, "%sdets%d.dat", path, my_rank);
        FILE *file_p = fopen(buffer, "rb");
        if (!file_p) {
            fprintf(stderr, "Error: could not open saved binary vector file at %s\n", buffer);
            return;
        }
        n_dets = fread(indices_, sizeof(long long), 10000000, file_p);
        fclose(file_p);
        
        sprintf(buffer, "%svals%d.dat", path, my_rank);
        file_p = fopen(buffer, "rb");
        if (!file_p) {
            fprintf(stderr, "Error: could not open saved binary vector file at %s\n", buffer);
            return;
        }
        fread(values_.data(), el_size, n_dets, file_p);
        fclose(file_p);
        
        size_t det_idx;
        n_nonz_ = 0;
        for (det_idx = 0; det_idx < n_dets; det_idx++) {
            int is_nonz = 0;
            if (fabs(values_[det_idx]) > 1e-9) {
                is_nonz = 1;
                values_[n_nonz_] = values_[det_idx];
            }
            if (is_nonz) {
                gen_orb_list(indices_[det_idx], tabl_, occ_orbs_[n_nonz_]);
                indices_[n_nonz_] = indices_[det_idx];
                matr_el_[n_nonz_] = NAN;
                n_nonz_++;
                if (n_sites_) {
                    find_neighbors_1D(indices_[det_idx], n_sites_, tabl_, n_elec_, neighb_[det_idx]);
                }
            }
        }
        curr_size_ = n_nonz_;
    }
    
    void collect_procs() {
        int n_procs = 1;
        int proc_idx;
        int my_rank = 0;
    #ifdef USE_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    #endif
        int vec_sizes[n_procs];
        vec_sizes[my_rank] = (int)curr_size_;
    #ifdef USE_MPI
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_UNSIGNED_LONG, vec_sizes, 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
        MPI_Datatype mpi_type;
    #endif
        int tot_size = 0;
        int disps[n_procs];
        for (proc_idx = 0; proc_idx < n_procs; proc_idx++) {
            disps[proc_idx] = tot_size;
            tot_size += vec_sizes[proc_idx];
        }
        size_t el_size = 0;
        if (type_ == DOUB) {
            el_size = sizeof(double);
    #ifdef USE_MPI
            mpi_type = MPI_DOUBLE;
    #endif
        }
        else if (type_ == INT) {
            el_size = sizeof(int);
    #ifdef USE_MPI
            mpi_type = MPI_INT;
    #endif
        }
        if (tot_size > max_size_) {
            indices_ = (long long *)realloc(indices_, sizeof(long long) * tot_size);
            values_.resize(tot_size);
        }
    #ifdef USE_MPI
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, indices_, vec_sizes, disps, MPI_LONG_LONG, MPI_COMM_WORLD);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, values_.data(), vec_sizes, disps, mpi_type, MPI_COMM_WORLD);
    #endif
        curr_size_ = tot_size;
    }
};


template <class el_type>
void Adder<el_type>::add(long long idx, el_type val, long long ini_flag) {
    int proc_idx = parent_vec_->idx_to_proc(idx);
    int *count = &send_cts_[proc_idx];
    if (*count == send_idx_.rows()) {
        enlarge_();
    }
    send_idx_(proc_idx, *count) = idx | ini_flag;
    send_vals_(proc_idx, *count) = val;
    (*count)++;
}

template <class el_type>
void Adder<el_type>::perform_add(long long ini_bit) {
        int n_procs = 1;
        int proc_idx;

        size_t el_size = 0;
        if (type_ == INT) {
            el_size = sizeof(int);
        }
        else if (type_ == DOUB) {
            el_size = sizeof(double);
        }
        else {
            fprintf(stderr, "Error: type %d not supported in function perform_add.\n", type_);
        }
#ifdef USE_MPI
        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Alltoall(send_cts_, 1, MPI_INT, recv_cts_, 1, MPI_INT, MPI_COMM_WORLD);
        MPI_Alltoallv(send_idx_[0], send_cts_, displacements_, MPI_LONG_LONG, recv_idx_[0], recv_cts_, displacements_, MPI_LONG_LONG, MPI_COMM_WORLD);
        MPI_Datatype mpi_type;
        if (type_ == INT) {
            mpi_type = MPI_INT;
        }
        else if (type_ == DOUB) {
            mpi_type = MPI_DOUBLE;
        }
        else {
            fprintf(stderr, "Error: type %d not supported in function perform_add.\n", type_);
        }
        MPI_Alltoallv(send_vals_[0], send_cts_, displacements_, mpi_type, recv_vals_[0], recv_cts_, displacements_, mpi_type, MPI_COMM_WORLD);
#else
        for (proc_idx = 0; proc_idx < n_procs; proc_idx++) {
            int cpy_size = send_cts_[proc_idx];
            recv_cts_[proc_idx] = cpy_size;
            memcpy(recv_idx_[proc_idx], send_idx_[proc_idx], cpy_size * sizeof(long long));
            memcpy(recv_vals_[proc_idx], send_vals_[proc_idx], cpy_size * el_size);
        }
#endif
        // Move elements from receiving buffers to vector
        for (proc_idx = 0; proc_idx < n_procs; proc_idx++) {
            send_cts_[proc_idx] = 0;
            parent_vec_->add_elements(recv_idx_[proc_idx], recv_vals_[proc_idx], recv_cts_[proc_idx], ini_bit);
        }
    }

/*! \brief Struct for storing a sparse vector */
//typedef struct {
//    long long *indices; ///< Array of indices of vector elements
//    void *values; ///< Array of values of vector elements
//    double *matr_el; ///< Array of pre-calculated diagonal matrix elements associated with each vector element
//    size_t max_size; ///< Maximum number of vector elements that can be stored
//    size_t curr_size; ///< Current number of vector elements stored
//    hash_table *vec_hash; ///< Hash table for quickly finding indices in the \p indices array
//    stack_entry *vec_stack; ///< Pointer to stack struct for managing available positions in the indices array
//    unsigned int *proc_scrambler; ///< Array of random numbers used in the hash function for assigning vector indices to MPI processes
//    byte_table *tabl; ///< Struct used to decompose determinant indices into lists of occupied orbitals
//    unsigned int n_elec; ///< Number of electrons represented by determinant bit-string indices
////    unsigned char *occ_orbs; ///< 2-D array containing lists of occupied orbitals for each determniant index (dimensions \p max_size x \p n_elec)
//    Matrix<unsigned char> *occ_orbs;
////    unsigned char *neighb; ///< Pointer to array containing information about empty neighboring orbitals for Hubbard model
//    Matrix<unsigned char> *neighb;
//    unsigned int n_sites; ///< Number of sites along one dimension of the Hubbard lattice, if applicable
//    dtype type; ///< Type of elements in vector
//    adder *my_adder; ///< Pointer to adder struct for buffered addition of elements distributed across MPI processes
//    int n_nonz; /// Current number of nonzero elements in vector
//} dist_vec;

//void push_stack(dist_vec *vec, size_t idx);
//ssize_t pop_stack(dist_vec *vec);


/*! \brief Set up a dist_vec struct
 *
 * \param [in] size         Maximum number of elements to be stored
 * \param [in] add_size     Maximum number of elements per processor to use in
 *                          adder buffers
 * \param [in] rn_ptr       Pointer to an mt_struct object for RN generation
 * \param [in] n_orb        Number of spatial orbitals in the basis (half the
 *                          length of the vector of random numbers for the
 *                          hash function for processors)
 * \param [in] n_elec       Number of electrons represented in each vector index
 * \param [in] vec_type     Data type of vector elements
 * \param [in] n_sites      If the orbitals in vector indices represent lattice
 *                          sites, the number of sites along one dimension. 0
 *                          otherwise.
 * \return pointer to the newly allocated struct
 */
//dist_vec *init_vec(size_t size, size_t add_size, mt_struct *rn_ptr, unsigned int n_orb,
//                   unsigned int n_elec, dtype vec_type, int n_sites);

/*! \brief Collect all of the vector elements from other MPI processes and accumulate them in the vectors on all processes
 *
 * \param [in, out] vec     A pointer to the dist_vec object on which to perform this operation
 */
//void collect_procs(dist_vec *vec);


/*! \brief Calculate dot product
 *
 * Calculates dot product of a vector distributed across potentially many MPI
 * processes with a local sparse vector (such that the local results could be
 * added)
 *
 * \param [in] vec          Struct containing the distributed vector
 * \param [in] idx2         Indices of elements in the local vector
 * \param [in] vals2         Values of elements in the local vector
 * \param [in] num2         Number of elements in the local vector
 * \param [in] hashes2      hash values of the indices of the local vector from
 *                          the hash table of vec
 * \return the value of the dot product
 */
//double vec_dot(dist_vec *vec, long long *idx2, double *vals2, size_t num2,
//               unsigned long long *hashes2);


/*! \brief Generate list of occupied orbitals from bit-string representation of
 *  a determinant
 *
 * This implementation uses the procedure in Sec. 3.1 of Booth et al. (2014)
 * \param [in] det          bit string to be parsed
 * \param [in] table        byte table structure generated by gen_byte_table()
 * \param [out] occ_orbs    Occupied orbitals in the determinant
 * \return number of 1 bits in the bit string
 */
unsigned char gen_orb_list(long long det, byte_table *table, unsigned char *occ_orbs);


/*! \brief Generate lists of occupied orbitals in a determinant that are
 *  adjacent to an empty orbital if the orbitals represent sites on a 1-D lattice
 *
 * \param [in] det          bit string representation of the determinant
 * \param [in] n_sites      number of sites in the lattice
 * \param [in] table        byte table struct generated by gen_byte_table()
 * \param [in] n_elec       number of electrons in the system
 * \param [out] neighbors   2-D array whose 0th row indicates orbitals with an
 *                          empty adjacent orbital to the left, and 1st row is
 *                          for empty orbitals to the right. Elements in the 0th
 *                          column indicate number of elements in each row.
 */
void find_neighbors_1D(long long det, unsigned int n_sites, byte_table *table,
                       unsigned int n_elec,
                       unsigned char *neighbors);


/*! \brief Hash function mapping vector index to processor
 *
 * \param [in] vec          Pointer to distributed sparse vector struct
 * \param [in] idx          Vector index
 * \return processor index from hash value
 */
//int idx_to_proc(dist_vec *vec, long long idx);


/*! \brief Hash function mapping vector index to local hash value
 *
 * The local hash value is used to find the index on a particular processor
 *
 * \param [in] vec          Pointer to distributed sparse vector struct
 * \param [in] idx          Vector index
 * \return hash value
 */
//unsigned long long idx_to_hash(dist_vec *vec, long long idx);


/*! \brief Add an int to a vector
 *
 * The element will be added to a buffer for later processing
 *
 * \param [in] vec          The dist_vec struct to which the element will be
 *                          added
 * \param [in] idx          The index of the element in the vector
 * \param [in] val          The value of the added element
 * \param [in] ini_flag     A bit string indicating whether the added element
 *                          came from an initiator element. None of the 1 bits
 *                          should overlap with the orbitals encoded in the rest
 *                          of the bit string
 */
//void add_int(dist_vec *vec, long long idx, int val, long long ini_flag);


/*! \brief Add a double to a vector
 *
 * The element will be added to a buffer for later processing
 *
 * \param [in] vec          The dist_vec struct to which the element will be
 *                          added
 * \param [in] idx          The index of the element in the vector
 * \param [in] val          The value of the added element
 * \param [in] ini_flag     A bit string indicating whether the added element
 *                          came from an initiator element. None of the 1 bits
 *                          should overlap with the orbitals encoded in the rest
 *                          of the bit string
 */
//void add_doub(dist_vec *vec, long long idx, double val, long long ini_flag);


/*! \brief Incorporate elements from the buffer into the vector
 *
 * Sign-coherent elements are added regardless of their corresponding initiator
 * flags. Otherwise, only elements with nonzero initiator flags are added
 *
 * \param [in] vec          The dist_vec struct on which to perform addition
 * \param [in] ini_bit      A bit mask defining where to look for initiator
 *                          flags in added elements
 */
//void perform_add(dist_vec *vec, long long ini_bit);


/*! \brief Delete an element from the vector
 *
 * Removes an element from the vector and modifies the hash table accordingly
 *
 * \param [in] vec          The dist_vec struct from which to delete the element
 * \param [in] pos          The position of the element to be deleted in the
 *                          element storage array of the dist_vec structure
 */
//void del_at_pos(dist_vec *vec, size_t pos);

/*! \brief Get a pointer to an element in the vector
 *
 * This function must be used in lieu of \p vec->values[\p pos] because the
 * values are stored as a (void *) array
 *
 * \param [in] vec          The dist_vec structure from which to read the element
 * \param [in] pos          The position of the desired element in the local
 *                          storage
 */
//int *int_at_pos(dist_vec *vec, size_t pos);


/*! \brief Get a pointer to an element in the vector
 *
 * This function must be used in lieu of \p vec->values[\p pos] because the
 * values are stored as a (void *) array
 *
 * \param [in] vec          The dist_vec structure from which to read the element
 * \param [in] pos          The position of the desired element in the local
 *                          storage
 */
//double *doub_at_pos(dist_vec *vec, size_t pos);


/*! \brief Get a pointer to the list of occupied orbitals corresponding to an
 * existing determinant index in the vector
 *
 * \param [in] vec          The dist_vec structure to reference
 * \param [in] pos          The position of the index in the local storage
 */
//unsigned char *orbs_at_pos(dist_vec *vec, size_t pos);


/*! \brief Calculate the one-norm of a vector
 *
 * This function sums over the elements on all MPI processes
 *
 * \param [in] vec          The vector whose one-norm is to be calculated
 * \return The one-norm of the vector
 */
//double local_norm(dist_vec *vec);


/*! Save a vector to disk in binary format
 *
 * The vector indices from each MPI process are stored in the file
 * [path]dets[MPI rank].dat, and the values at [path]vals[MPI rank].dat
 *
 * \param [in] vec          Pointer to the vector to save
 * \param [in] path         Location where the files are to be stored
 */
//void save_vec(dist_vec *vec, const char *path);


/*! Load a vector from disk in binary format
 *
 * The vector indices from each MPI process are read from the file
 * [path]dets[MPI rank].dat, and the values from [path]vals[MPI rank].dat
 *
 * \param [out] vec         Pointer to an allocated and initialized dist_vec
 *                          struct
 * \param [in] path         Location where the files are to be stored
 */
//void load_vec(dist_vec *vec, const char *path);

//#ifdef __cplusplus
//}
//#endif

#endif /* vec_utils_h */
