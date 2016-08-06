#ifndef __CONCURRENT_CIR_QUEUE
#define __CONCURRENT_CIR_QUEUE

#include <mutex>
#include <condition_variable>
#include <boost/circular_buffer.hpp>

// See http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html

template<typename Data>
class ConcurrentCirQueue
{
public:
    ConcurrentCirQueue()
    {}

    ConcurrentCirQueue(int capacity) : cir_buffer_(capacity)
    {}

    void SetCapacity(int capacity)
    {
        cir_buffer_.set_capacity(capacity);
    }

    void PushBack(Data const& data)
    {
        {
            std::lock_guard<std::mutex> lock(the_mutex_);
            cir_buffer_.push_back(data);
        }
        the_condition_variable_.notify_one();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(the_mutex_);
        return cir_buffer_.empty();
    }

    bool TryPopFront(Data& popped_value)
    {
        std::lock_guard<std::mutex> lock(the_mutex_);
        if(cir_buffer_.empty()) {
            return false;
        }
        popped_value = cir_buffer_.front();
        cir_buffer_.pop_front();
        return true;
    }

    void WaitAndPopFront(Data& popped_value)
    {
        std::lock_guard<std::mutex> lock(the_mutex_);
        while(cir_buffer_.empty()) {
            the_condition_variable_.wait(lock);
        }
        popped_value = cir_buffer_.front();
        cir_buffer_.pop_front();
    }

private:
    boost::circular_buffer<Data> cir_buffer_;
    mutable std::mutex the_mutex_;
    std::condition_variable the_condition_variable_;
};

#endif // __CONCURRENT_CIR_QUEUE
