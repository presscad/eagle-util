#ifndef _SIMPLE_THREAD_POOL_H
#define _SIMPLE_THREAD_POOL_H

#include <memory>
#include <deque>
#include <thread>
#include <mutex>
#include <sstream>
#include <condition_variable>

namespace util {

// SimpleDataQueue's get method returns immediately when queue is empty
template<class T>
struct SimpleDataQueue
{
    void Add(const T& data)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push_back(data);
    }

    void Add(T&& data)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push_back(std::forward<T>(data));
    }

    bool Get(T& data)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (queue_.empty()) {
            return false;
        }
        data = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.size();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.empty();
    }

    // get the internal queue, whose type may be changed in the future
    // also, the internal data will be exposed without mutex protection
    // use it at your own risk!
    const std::deque<T>& InternalQueue() const
    {
        return queue_;
    }

    std::deque<T>& InternalQueue()
    {
        return queue_;
    }

private:
    std::deque<T> queue_;
    mutable std::mutex queue_mutex_;
};


// BlockingDataQueue's get method is blocked when queue is empty
template<class T>
struct BlockingDataQueue
{
    BlockingDataQueue()
    {}

    void Add(const T& data)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_.push_back(data);
        lock.unlock();
        condition_var_.notify_one();
    }

    void Add(T&& data)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_.push_back(std::forward<T>(data));
        lock.unlock();
        condition_var_.notify_one();
    }

    bool Get(T& data)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!active_) {
            return false;
        }
        while (queue_.empty()) {
            condition_var_.wait(lock);
            if (!active_) {
                return false;
            }
        }
        data = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.size();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.empty();
    }

    // Destory the queue and notify all the wating threads to wake up
    void Destory()
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_.clear();
        active_ = false;
        lock.unlock();
        condition_var_.notify_all();
    }

    // get the internal queue, whose type may be changed in the future
    // also, the internal data will be exposed without mutex protection
    // use it at your own risk!
    const std::deque<T>& InternalQueue() const
    {
        return queue_;
    }

    std::deque<T>& InternalQueue()
    {
        return queue_;
    }

private:
    std::deque<T> queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_var_;
    bool active_ = {true};
};


class SimpleThreadPool
{
public:
    struct ThreadInfo
    {
        explicit ThreadInfo()
        {}

        explicit ThreadInfo(SimpleThreadPool* p_pool)
            : p_pool_(p_pool)
        {}

        std::string IdToStr() const
        {
            std::stringstream ss;
            ss << p_thread_->get_id();
            std::string str;
            ss >> str;
            return str;
        }

        const std::thread& GetThread() const
        {
            return *p_thread_;
        }

    private:
        std::shared_ptr<std::thread> p_thread_;
        SimpleThreadPool* p_pool_ = {};
        friend class SimpleThreadPool;
    };

public:
    explicit SimpleThreadPool(size_t thread_num)
    {
        for (size_t i = 0; i < thread_num; ++i) {
            threads_.push_back(ThreadInfo(this));
        }
    }

    template<class Function, class... Args>
    explicit SimpleThreadPool(size_t thread_num, Function&& f, Args&&... args)
    {
        for (size_t i = 0; i < thread_num; ++i) {
            threads_.push_back(ThreadInfo(this));
        }
        SetThreadFunction(std::forward<Function>(f), std::forward<Args>(args)...);
    }

    template<class Function, class... Args>
    void SetThreadFunction(Function&& f, Args&&... args)
    {
        for (auto& t : threads_) {
            t.p_thread_ = std::make_shared<std::thread>(
                std::forward<Function>(f), std::forward<Args>(args)...);
        }
    }

    template<class Function>
    explicit SimpleThreadPool(size_t thread_num, Function&& f)
    {
        for (size_t i = 0; i < thread_num; ++i) {
            threads_.push_back(ThreadInfo(this));
        }
        SetThreadFunction(std::forward<Function>(f));
    }

    template<class Function>
    void SetThreadFunction(Function&& f)
    {
        for (auto& t : threads_) {
            t.p_thread_ = std::make_shared<std::thread>(
                std::forward<Function>(f));
        }
    }

    void JoinAll()
    {
        for (auto& t : threads_) {
            t.p_thread_->join();
        }
    }

    size_t ThreadNum() const
    {
        return threads_.size();
    }

    // Get the pointer to ThreadInfo structure whose thread ID is the current thread ID.
    // Return nullptr if the current thread is not in the pool.
    const ThreadInfo* GetCurrentThreadInfo() const
    {
        for (auto& t : threads_) {
            if (t.p_thread_->get_id() == std::this_thread::get_id()) {
                return &t;
            }
        }
        return nullptr;
    }

private:
    std::vector<ThreadInfo> threads_;
};

// Create a simple thread pool
// TaskFun: void thread_fun()
template <typename TaskFun>
SimpleThreadPool CreateSimpleThreadPool(const std::string &/*task_name*/, unsigned task_count, TaskFun task_fun)
{
    if (task_count == 0) {
        task_count = std::thread::hardware_concurrency();
        if (task_count == 0) task_count = 2;
    }
    return SimpleThreadPool(task_count, task_fun);
}

}
#endif //_SIMPLE_THREAD_POOL_H
