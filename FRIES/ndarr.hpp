/*! \file C++ definitions of classes for variable-length multidimensional arrays
 * Largely copied from https://isocpp.org/wiki/faq/operator-overloading#matrix-subscript-op
 */

#ifndef ndarr_h
#define ndarr_h

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <FRIES/math_utils.h>

/*! \brief A class for representing and manipulating matrices with variable dimension sizes
 * \tparam mat_type The type of elements to be stored in the matrix
 */
template <class mat_type>
class Matrix {
public:
    /*! \brief Constructor for Matrix class
     * \param [in] rows     The number of rows the matrix should have initially
     * \param [in] cols     The number of columns the matrix should have initially
     */
    Matrix(size_t rows, size_t cols) : rows_(rows), cols_(cols), tot_size_(rows * cols), data_(rows * cols) {}
    
    /*! \brief Access matrix element
     * \param [in] row      Row index of element
     * \param [in] col      Column index of element
     * \return Reference to matrix element
     */
    mat_type& operator() (size_t row, size_t col) {
        return data_[cols_*row + col];
    }
    
    /*! \brief Access matrix element
     * \param [in] row      Row index of element
     * \param [in] col      Column index of element
     * \return matrix element
     */
    mat_type  operator() (size_t row, size_t col) const {
        return data_[cols_ * row + col];
    }
    
    /*! \brief Access matrix row
     * \param [in] row      Row index
     * \return pointer to 0th element in a row of a matrix
     */
    mat_type *operator[] (size_t row) {
        return &data_[cols_ * row];
    }
    
    /*! \brief Access matrix row
     * \param [in] row      Row index
     * \return pointer to 0th element in a row of a matrix
     */
    const mat_type *operator[] (size_t row) const {
        return &data_[cols_ * row];
    }
    
    /*! \brief Increase number of columns in the matrix
     * Data are copied such that the first n[i] elements in each row remain the same before and after this operation
     * \param [in] new_col      Desired number of columns in the enlarged matrix
     * \param [in] n_keep       Number of elements to preserve in each row of the matrix (should have \p rows_ elements)
     */
    void enlarge_cols(size_t new_col, int *n_keep) {
        if (new_col > cols_) {
            size_t old_cols = cols_;
            reshape(rows_, new_col);
            
            size_t row_idx;
            for (row_idx = rows_; row_idx > 0; row_idx--) {
                auto begin = data_.begin();
                std::copy_backward(begin + (row_idx - 1) * old_cols, begin + (row_idx - 1) * old_cols + n_keep[row_idx - 1], begin + (row_idx - 1) * new_col + n_keep[row_idx - 1]);
            }
        }
    }
    
    
    /*! \brief Change the dimensions without moving any of the data
     * \param [in] new_rows     Desired number of rows in the reshaped matrix
     * \param [in] new_cols     Desired number of columns in the reshaped matrix
     */
    void reshape(size_t new_rows, size_t new_cols) {
        size_t new_size = new_rows * new_cols;
        if (new_size > tot_size_) {
            tot_size_ = new_size;
            data_.resize(tot_size_);
        }
        rows_ = new_rows;
        cols_ = new_cols;
    }
    
    Matrix(const Matrix& m) = delete;
    Matrix& operator= (const Matrix& m) = delete;
    
    /*! \return Current number of rows in matrix */
    size_t rows() const {
        return rows_;
    }
    /*! \return Current number of columns in matrix*/
    size_t cols() const {
        return cols_;
    }
    
    /*! \return Pointer to the  data in the matrix*/
    mat_type *data() const {
        return (mat_type *) data_.data();
    }
    
private:
    size_t rows_, cols_, tot_size_;
    std::vector<mat_type> data_;
};


/*! \brief A class for storing 4-D arrays of doubles*/
class FourDArr {
public:
    /*! \brief Constructor
     * \param [in] len1 len2 len3 len4    Lengths of the 4 dimensions of the array
     */
    FourDArr(size_t len1, size_t len2, size_t len3, size_t len4)
    : len1_(len1)
    , len2_(len2)
    , len3_(len3)
    , len4_(len4)
    {
        data_ = (double *)malloc(sizeof(double) * len1 * len2 * len3 * len4);
    }
    
    /*! \brief Access an element of the 4-D array
     * \param [in] i1 First index
     * \param [in] i2 Second index
     * \param [in] i3 Third index
     * \param [in] i4 Fourth index
     * \returns Reference to array element
     */
    double& operator() (size_t i1, size_t i2, size_t i3, size_t i4) {
        return data_[i1 * len2_ * len3_ * len4_ + i2 * len3_ * len4_ + i3 * len4_ + i4];
    }
    
    /*! \brief Access an element of the 4-D array
     * \param [in] i1 First index
     * \param [in] i2 Second index
     * \param [in] i3 Third index
     * \param [in] i4 Fourth index
     * \returns Array element
     */
    double  operator() (size_t i1, size_t i2, size_t i3, size_t i4) const {
        return data_[i1 * len2_ * len3_ * len4_ + i2 * len3_ * len4_ + i3 * len4_ + i4];
    }
    
    /*! \brief Destructor*/
    ~FourDArr() {
        free(data_);
    }
    
    FourDArr(const FourDArr& m) = delete;
    
    FourDArr& operator= (const FourDArr& m) = delete;
    
    /*! \returns pointer to 0th element in the array */
    double *data() {
        return data_;
    }
private:
    size_t len1_, len2_, len3_, len4_; ///< Dimensions of the array
    double* data_; ///< The data stored in the array
};

template<> class Matrix<bool>  {
    std::vector<uint64_t> data_;
    size_t rows_, cols_, tot_size_, cols_coarse_;
    
    class bool_reference
    {
    private:
        uint64_t & value_;
        uint64_t mask_;
        
    public:
        bool_reference(uint64_t & value, uint8_t nbit)
        : value_(value), mask_(uint64_t(0x1) << nbit)
        { }
        
        void zero(void) noexcept { value_ &= ~(mask_); }
        
        void one(void) noexcept { value_ |= (mask_); }
        
        bool get() const noexcept { return !!(value_ & mask_); }
        
        void set(bool b) noexcept
        {
            if(b)
                one();
            else
                zero();
        }
        
        bool_reference & operator=(bool b) noexcept { set(b); return *this; }
        
        bool_reference & operator=(const bool_reference & br) noexcept { return *this = bool(br); }
        
        operator bool() const noexcept { return get(); }
        };
        
    public:
        Matrix(size_t rows, size_t cols) : rows_(rows), cols_(cols), cols_coarse_(CEILING(cols, 64)), tot_size_(rows * CEILING(cols, 64)), data_(rows * CEILING(cols, 64), 0) {
        }
        
        bool_reference operator() (size_t row, size_t col) {
            bool_reference ref(data_[cols_coarse_ * row + col / 64], col % 64);
            return ref;
        }
        
        /*! \brief Change the dimensions without moving any of the data
         * \param [in] new_rows     Desired number of rows in the reshaped matrix
         * \param [in] new_cols     Desired number of columns in the reshaped matrix
         */
        void reshape(size_t new_rows, size_t new_cols) {
            cols_coarse_ = CEILING(new_cols, 64);
            size_t new_size = new_rows * cols_coarse_;
            if (new_size > tot_size_) {
                tot_size_ = new_size;
                data_.resize(tot_size_, 0);
            }
            rows_ = new_rows;
            cols_ = new_cols;
        }
        
        /*! \return Current number of columns in matrix*/
        size_t cols() const {
            return cols_;
        }
        
        /*! \brief Access matrix row
         * \param [in] row      Row index
         * \return pointer to 0th element in a row of a matrix
         */
        uint64_t *operator[] (size_t row) {
            return &data_[cols_coarse_ * row];
        }
        
        /*! \brief Access matrix row
         * \param [in] row      Row index
         * \return pointer to 0th element in a row of a matrix
         */
        const uint64_t *operator[] (size_t row) const {
            return &data_[cols_coarse_ * row];
        }
    };
    
#endif /* ndarr_h */
