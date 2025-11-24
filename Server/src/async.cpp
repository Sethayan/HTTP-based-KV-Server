#include "async.h"
#include <iostream>
#include <sstream>

AsyncWriter::AsyncWriter(MySQLPool *pool)
    : dbpool_(pool), running_(false) {}

AsyncWriter::~AsyncWriter() {
    stop();
}

void AsyncWriter::start() {
    running_ = true;
    worker_ = std::thread(&AsyncWriter::worker_loop, this);
}

void AsyncWriter::stop() {
    if (!running_) return;
    running_ = false;
    cv_.notify_all();
    if (worker_.joinable())
        worker_.join();
}

void AsyncWriter::async_insert(const std::string &key, const std::string &value) {
    {
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push({AsyncOpType::INSERT_OP, key, value});
    }
    cv_.notify_one();
}

void AsyncWriter::async_delete(const std::string &key) {
    {
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push({AsyncOpType::DELETE_OP, key, ""});
    }
    cv_.notify_one();
}

// =============================================================
// worker_loop: runs in background thread
// =============================================================
void AsyncWriter::worker_loop() {
    while (running_) {
        AsyncTask task;

        // Wait for work
        {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [&]{ return !queue_.empty() || !running_; });

            if (!running_) break;
            task = queue_.front();
            queue_.pop();
        }

        // Process task
        MYSQL *conn = dbpool_->acquire();

        if (task.type == AsyncOpType::INSERT_OP) {
            std::string esk, esv;
            esk.resize(task.key.size()*2 + 1);
            esv.resize(task.value.size()*2 + 1);

            unsigned long klen = mysql_real_escape_string(conn, &esk[0], task.key.c_str(), task.key.size());
            unsigned long vlen = mysql_real_escape_string(conn, &esv[0], task.value.c_str(), task.value.size());

            esk.resize(klen);
            esv.resize(vlen);

            std::stringstream q;
            q << "INSERT INTO kvstore (k,hash,v) VALUES ('"
              << esk << "', 0, '" << esv << "') "
              << "ON DUPLICATE KEY UPDATE v=VALUES(v), updated=CURRENT_TIMESTAMP";

            if (mysql_query(conn, q.str().c_str())) {
                std::cerr << "[AsyncWriter] Insert error: " << mysql_error(conn) << "\n";
            }
        }
        else if (task.type == AsyncOpType::DELETE_OP) {
            std::string esk;
            esk.resize(task.key.size()*2 + 1);

            unsigned long klen = mysql_real_escape_string(conn, &esk[0], task.key.c_str(), task.key.size());
            esk.resize(klen);

            std::string q = "DELETE FROM kvstore WHERE k='" + esk + "'";
            if (mysql_query(conn, q.c_str())) {
                std::cerr << "[AsyncWriter] Delete error: " << mysql_error(conn) << "\n";
            }
        }

        dbpool_->release(conn);
    }
}
