#ifndef KV_DBPOOL_H
#define KV_DBPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

// Simple thread-safe MySQL connection pool.
//  - acquire(): blocks until a free connection.
//  - release(): returns conn back to pool.
//  - pool is fixed size.
//  - used by async writer and main server handlers.

class MySQLPool {
public:
    MySQLPool(const std::string &host,
              const std::string &user,
              const std::string &pass,
              const std::string &db,
              unsigned int port = 3306,
              size_t pool_size = 8);

    ~MySQLPool();

    // get a connection (waits if none available)
    MYSQL* acquire();

    // release connection back into pool
    void release(MYSQL *conn);

    // error reporting
    std::string last_error() const;

private:
    bool create_pool();
    void close_pool();

private:
    std::string host_;
    std::string user_;
    std::string pass_;
    std::string db_;
    unsigned int port_;
    size_t pool_size_;

    mutable std::string last_err_;

    std::vector<MYSQL*> conns_;   // available connections
    std::mutex mu_;
    std::condition_variable cv_;
};

#endif // KV_DBPOOL_H
