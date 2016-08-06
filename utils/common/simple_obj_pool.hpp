
#ifndef _SIMPLE_OBJ_POOL_HPP_
#define _SIMPLE_OBJ_POOL_HPP_

#include <vector>
#include <mutex>

namespace util {

template<typename T>
class SimpleObjPool {
public:
    void Reserve(size_t capacity)
    {
        pool_.reserve(capacity);
    }

    size_t Capacity() const
    {
        return pool_.capacity();
    }

    size_t Size() const
    {
        return pool_.size();
    }

    // Different from AllocNewIndex(), it fails if no enough capacity
    T* AllocNew()
    {
        if (pool_.size() >= pool_.capacity()) {
            return nullptr;
        }
        pool_.push_back(T());
        return &pool_.back();
    }

    T* AllocNew(const T& val)
    {
        if (pool_.size() >= pool_.capacity()) {
            return nullptr;
        }
        pool_.push_back(val);
        return &pool_.back();
    }

    T* AllocNew(T&& val)
    {
        if (pool_.size() >= pool_.capacity()) {
            return nullptr;
        }
        pool_.push_back(std::forward<T>(val));
        return &pool_.back();
    }

    // AllocNewIndex return the index of the new object
    // Different from AllocNew, the pool will be automatically enlarged if not enough capacity
    int AllocNewIndex()
    {
        pool_.push_back(T());
        return (int)pool_.size() - 1;
    }

    int AllocNewIndex(const T& val)
    {
        pool_.push_back(val);
        return (int)pool_.size() - 1;
    }

    int AllocNewIndex(T&& val)
    {
        pool_.push_back(std::forward<T>(val));
        return (int)pool_.size() - 1;
    }

    T* ObjPtrByIndex(int index)
    {
        return &pool_.at(index);
    }

    const T* ObjPtrByIndex(int index) const
    {
        return &pool_.at(index);
    }

    T& operator[](int index)
    {
        return pool_[index];
    }

    const T& operator[](int index) const
    {
        return pool_[index];
    }

    void Clear()
    {
        pool_.clear();
    }

    void Destory()
    {
        pool_ = std::move(std::vector<T>());
    }

    const std::vector<T>& AllObjs() const
    {
        return pool_;
    }
    std::vector<T>& AllObjs()
    {
        return pool_;
    }

private:
    std::vector<T> pool_;
};
}

#endif // _SIMPLE_OBJ_POOL_HPP_
