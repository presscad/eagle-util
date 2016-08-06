#ifndef __CONCURRENT_QUEUE
#define __CONCURRENT_QUEUE

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/circular_buffer.hpp>

// See http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html

template<typename Data>
class ConcurrentCirQueue
{
public:
    ConcurrentCirQueue() {
    }

    ConcurrentCirQueue(int capacity) : cir_buffer(capacity) {
    }

    void SetCapacity(int capacity) {
        cir_buffer.set_capacity(capacity);
    }

    void push_back(Data const& data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        cir_buffer.push_back(data);
        lock.unlock();
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return cir_buffer.empty();
    }

    bool try_pop_front(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        if(cir_buffer.empty()) {
            return false;
        }
        popped_value = cir_buffer.front();
        cir_buffer.pop_front();
        return true;
    }

    void wait_and_pop_front(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        while(cir_buffer.empty()) {
            the_condition_variable.wait(lock);
        }
        popped_value = cir_buffer.front();
        cir_buffer.pop_front();
    }

private:
    boost::circular_buffer<Data> cir_buffer;
    mutable boost::mutex the_mutex;
    boost::condition_variable the_condition_variable;
};

#endif // __CONCURRENT_QUEUE
