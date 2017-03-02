#ifndef _SIMPLE_CONN_POOL_H
#define _SIMPLE_CONN_POOL_H

#include <memory>
#include <vector>
#include <mutex>

namespace util {

template <typename CONNECTION> class SimpleConnPool;

template <typename CONNECTION>
class ConnectionWrapper
{
public:
    typedef SimpleConnPool<CONNECTION> ConnPoolType;

    explicit ConnectionWrapper(ConnPoolType* p_pool)
        : p_pool_(p_pool)
    {}

    ~ConnectionWrapper()
    {
        ReturnToPool();
    }

    CONNECTION* Get() const
    {
        return p_raw_conn_;
    }

    // return the connection object to the pool, available for later "get" from the pool
    bool ReturnToPool()
    {
        bool res = false;
        if (p_raw_conn_) {
            res = p_pool_->ReturnConnection(*this);
            p_raw_conn_ = nullptr;
        }

        return false;
    }

    // destory (close) the connection object. also remove it from the pool
    void Destory()
    {
        if (p_raw_conn_) {
            p_pool_->RemoveConnection(*this);

            delete p_raw_conn_;
            p_raw_conn_ = false;
        }
    }

private:
    ConnPoolType* p_pool_{};
    CONNECTION *p_raw_conn_{};

    template<typename CONNECTION> friend class SimpleConnPool;
};


// usage example:
// OdbcConn* OdbcCreateFun()
// {
//     OdbcConn *p_conn = new OdbcConn(gConfigs.db.dsn.c_str(), ...
//     ...
//     return p_conn;
// };
// util::SimpleConnPool<OdbcConn> g_odbc_conn_pool(4, OdbcCreateFun);
template <typename CONNECTION>
class SimpleConnPool
{
public:
    typedef CONNECTION* (*ConnCreateFun)();

    explicit SimpleConnPool(int max_conn, ConnCreateFun fun)
        : max_conn_(max_conn), create_fun_(fun)
    {}

    ~SimpleConnPool()
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        for (auto& iconn : internal_conns_) {
            delete iconn.p_raw_conn;
            iconn.p_raw_conn = nullptr;
        }
        internal_conns_.clear();
    }

    std::shared_ptr<ConnectionWrapper<CONNECTION>> GetConnection()
    {
        auto p_conn_wrapper = std::make_shared<ConnectionWrapper<CONNECTION>>(this);

        if (internal_conns_.empty()) {
            InternalConn iconn;
            iconn.p_raw_conn = create_fun_();
            if (nullptr == iconn.p_raw_conn) {
                return false;
            }
            iconn.use_flag = true;

            std::lock_guard<std::mutex> lguard(queue_mutex_);
            internal_conns_.push_back(iconn);
            p_conn_wrapper->p_raw_conn_ = iconn.p_raw_conn;
            return p_conn_wrapper;
        }
        else {
            std::lock_guard<std::mutex> lguard(queue_mutex_);
            for (auto& iconn : internal_conns_) {
                if (iconn.use_flag == false) {
                    iconn.use_flag = true;

                    p_conn_wrapper->p_raw_conn_ = iconn.p_raw_conn;
                    return p_conn_wrapper;
                }
            }
        }

        if ((int)internal_conns_.size() < max_conn_) {
            InternalConn iconn;
            iconn.p_raw_conn = create_fun_();
            if (nullptr == iconn.p_raw_conn) {
                return false;
            }
            iconn.use_flag = true;

            std::lock_guard<std::mutex> lguard(queue_mutex_);
            internal_conns_.push_back(iconn);
            p_conn_wrapper->p_raw_conn_ = iconn.p_raw_conn;
            return p_conn_wrapper;
        }

        return nullptr;
    }

private:
    struct InternalConn
    {
        CONNECTION *p_raw_conn{};
        bool use_flag{};
    };

    bool RemoveConnection(ConnectionWrapper<CONNECTION>& conn_wrapper)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        for (int i = 0; i < (int)internal_conns_.size(); ++i) {
            if (internal_conns_[i].p_raw_conn == conn_wrapper.Get()) {
                internal_conns_.erase(internal_conns_.begin() + i);
                return true;
            }
        }

        return false;
    }

    bool ReturnConnection(ConnectionWrapper<CONNECTION>& conn_wrapper)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        for (auto& iconn : internal_conns_) {
            if (iconn.p_raw_conn == conn_wrapper.Get()) {
                iconn.use_flag = false;
                return true;
            }
        }

        return false;
    }

    int max_conn_{};
    ConnCreateFun create_fun_;
    std::vector<InternalConn> internal_conns_;
    mutable std::mutex queue_mutex_;

    template<typename CONN> friend class ConnectionWrapper;
};

} // end of namespace
#endif //_SIMPLE_CONN_POOL_H
