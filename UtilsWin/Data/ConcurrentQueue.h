#ifndef __CONCURRENT_QUEUE
#define __CONCURRENT_QUEUE

#include <queue>
#include <mutex>
#include <thread>


// See http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html

template<typename Data>
class concurrent_queue
{
private:
    std::queue<Data> the_queue;
    mutable std::mutex the_mutex;
    std::condition_variable the_condition_variable;

public:
    void push(Data const& data)
    {
        std::unique_lock<std::mutex> lock(the_mutex, std::try_to_lock);
        the_queue.push(data);
        lock.unlock();
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> lock(the_mutex, std::try_to_lock);
        return the_queue.empty();
    }

    bool try_pop(Data& popped_value)
    {
        std::unique_lock<std::mutex> lock(the_mutex, std::try_to_lock);
        if(the_queue.empty())
        {
            return false;
        }
        
        popped_value=the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        std::unique_lock<std::mutex> lock(the_mutex, std::try_to_lock);
        while(the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }
        
        popped_value=the_queue.front();
        the_queue.pop();
    }
};

#endif // __CONCURRENT_QUEUE
