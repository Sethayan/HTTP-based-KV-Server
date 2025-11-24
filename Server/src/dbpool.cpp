#include "dbpool.h"
#include <iostream>

MySQLPool::MySQLPool(const std::string &host,
                     const std::string &user,
                     const std::string &pass,
                     const std::string &db,
                     unsigned int port,
                     size_t pool_size)
    : host_(host),
      user_(user),
      pass_(pass),
      db_(db),
      port_(port),
      pool_size_(pool_size)
{
    if (!create_pool()) {
        throw std::runtime_error("MySQLPool failed: " + last_err_);
    }
}

MySQLPool::~MySQLPool() {
    close_pool();
}

bool MySQLPool::create_pool() {
    for (size_t i = 0; i < pool_size_; i++) {

        MYSQL *conn = mysql_init(nullptr);
        if (!conn) {
            last_err_ = "mysql_init failed";
            return false;
        }

        // ---------------------------------------------------------
        // ENABLE AUTO-RECONNECT (MySQL8 compatible)
        // ---------------------------------------------------------
        bool reconnect = true;
        if (mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect)) {
            last_err_ = "mysql_options MYSQL_OPT_RECONNECT failed";
            mysql_close(conn);
            return false;
        }

        // ---------------------------------------------------------
        // CONNECT
        // ---------------------------------------------------------
        if (!mysql_real_connect(conn,
                                host_.c_str(),
                                user_.c_str(),
                                pass_.c_str(),
                                db_.c_str(),
                                port_,
                                nullptr,
                                0))
        {
            last_err_ = mysql_error(conn);
            mysql_close(conn);
            return false;
        }

        // ---------------------------------------------------------
        // UTF8 encoding
        // ---------------------------------------------------------
        mysql_set_character_set(conn, "utf8mb4");

        conns_.push_back(conn);
    }

    return true;
}

void MySQLPool::close_pool() {
    std::lock_guard<std::mutex> lk(mu_);

    for (MYSQL *conn : conns_) {
        if (conn)
            mysql_close(conn);
    }
    conns_.clear();
}

MYSQL* MySQLPool::acquire() {
    std::unique_lock<std::mutex> lk(mu_);

    // wait for available connection
    cv_.wait(lk, [&]{ return !conns_.empty(); });

    MYSQL *c = conns_.back();
    conns_.pop_back();
    return c;
}

void MySQLPool::release(MYSQL *conn) {
    std::lock_guard<std::mutex> lk(mu_);
    conns_.push_back(conn);
    cv_.notify_one();
}

std::string MySQLPool::last_error() const {
    return last_err_;
}
