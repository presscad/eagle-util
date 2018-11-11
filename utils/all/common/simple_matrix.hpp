/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Author      : SAP Custom Development
 * Description : Simple matrix class
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20140806  Initial creation
 *----------------------------------------------------------------------*/

#ifndef _SIMPLE_MATRIX_HPP_
#define _SIMPLE_MATRIX_HPP_

#include <vector>


template<typename T>
class SimpleMatrix {
public:
    typedef T value_type;

    explicit SimpleMatrix() : height_(0), width_(0)
    {
    }

    explicit SimpleMatrix(int height, int width) : height_(height), width_(width),
        data_(height * width)
    {
        SetSize(height, width);
    }

    void SetSize(int height, int width) {
        // Preferred to be only called once
        width_ = width;
        height_ = height;
        int total = width * height;
        data_.resize(total);
    }

    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }

    int size1() const {
        return height_;
    }
    int size2() const {
        return width_;
    }

    const T& operator() (int i, int j) const {
        return data_[i * width_ + j];
    }

    T& operator() (int i, int j) {
        return data_[i * width_ + j];
    }

    void SetValue(int i, int j, const T& value) {
        data_[i * width_ + j] = value;
    }
    const T GetValue(int i, int j) const {
        return data_[i * width_ + j];
    }

protected:
    int height_, width_;
    std::vector<T> data_;
};

template<typename T>
class SimpleVector {
public:
    typedef T value_type;

    SimpleVector(const SimpleVector<T>& src)
    {
        data_ = src.data_;
    }

    explicit SimpleVector(int size)
    {
        data_.resize(size);
    }

    int size() const {
        return (int)data_.size();
    }
    const value_type& operator[](int k) const {
        return data_[k];
    }
    T& operator[](int k) {
        return data_[k];
    }

protected:
    std::vector<T> data_;
};

template<class M>
class SimpleMatrixRow {
public:
    typedef typename M::value_type value_type;
    typedef typename M::value_type* iterator;

    SimpleMatrixRow(M& m, int i) : p_m_(&m), i_(i) {
        width_ = m.width();
        height_ = m.height();
    }
    ~SimpleMatrixRow() {
        p_m_ = NULL;
    }
    iterator begin() const {
        return &(*p_m_)(i_, 0);
    }
    iterator end() const {
        return begin() + width_;
    }

    value_type & operator[](int j) {
        return (*p_m_)(i_, j);
    }
    const value_type & operator[](int j) const {
        return (*p_m_)(i_, j);
    }

    void swap(SimpleMatrixRow<M> &row) {
        for (int j = 0; j < width_; ++j) {
            value_type tmp = (*this)[j];
            (*this)[j] = row[j];
            row[j] = tmp;
        }
    }
    int width() const {
        return width_;
    }

    SimpleMatrixRow<M>& operator-=(SimpleMatrixRow<M>& row) {
        for (int j = 0; j < width_; ++j) {
            (*this)[j] -= row[j];
        }
        return *this;
    }
    SimpleMatrixRow<M>& operator-=(SimpleVector<value_type>& vector) {
        for (int j = 0; j < width_; ++j) {
            (*this)[j] -= vector[j];
        }
        return *this;
    }

    SimpleMatrixRow<M>& operator*=(value_type v) {
        for (int j = 0; j < width_; ++j) {
            (*this)[j] *= v;
        }
        return *this;
    }
    SimpleVector<value_type> operator* (value_type v) const {
        SimpleVector<value_type> vector(width_);
        for (int j = 0; j < width_; ++j) {
            vector[j] = (*this)[j] * v;
        }
        return vector;
    }

protected:
    M* p_m_;
    int i_;
    int width_, height_;
};


template <class T> class SimpleMatrixCol_Iter;

template<class M>
class SimpleMatrixCol {
public:
    typedef typename M::value_type value_type;
    typedef SimpleMatrixCol_Iter<typename M::value_type> iterator;

    SimpleMatrixCol(M& m, int j) : p_m_(&m), j_(j) {
        width_ = m.width();
        height_ = m.height();
    }
    ~SimpleMatrixCol() {
        p_m_ = NULL;
    }

    iterator begin() const {
        return iterator(&(*p_m_)(0, j_), width_);
    }
    iterator end() const {
        return begin() + height_;
    }

    value_type & operator[](int i) {
        return (*p_m_)(i, j_);
    }
    const value_type & operator[](int i) const {
        return (*p_m_)(i, j_);
    }

    void swap(SimpleMatrixCol<M> &col) {
        for (int k = 0; k < height_; ++k) {
            value_type tmp = (*this)[k];
            (*this)[k] = col[k];
            col[k] = tmp;
        }
    }

protected:
    M* p_m_;
    int j_;
    int width_, height_;

    friend class SimpleMatrixCol_Iter<class T>;
};

template <class T>
class SimpleMatrixCol_Iter {
public:
    typedef SimpleMatrixCol_Iter<T> iterator;

    SimpleMatrixCol_Iter(T *pointer, int width) : pointer_(pointer), width_(width) {
    }

    T & operator*() {
        return *pointer_;
    }
    iterator & operator++() {
        pointer_ += width_;
        return *this;
    }
    iterator operator++ (int)
    {
        SimpleMatrixCol_Iter<T> clone( *this );
        pointer_ += width_;
        return clone;
    }

    iterator operator+ (int k) const {
        SimpleMatrixCol_Iter<T> clone( *this );
        clone.pointer_ += width_ * k;
        return clone;
    }
    int operator-(iterator &it) const {
        return int(pointer_ - it.pointer_) / width_;
    }
    bool operator!= (iterator &it) const {
        return pointer_ != it.pointer_;
    }

protected:
    T* pointer_;
    int width_;
};

#endif // _SIMPLE_MATRIX_HPP_
