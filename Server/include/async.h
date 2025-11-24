#ifndef KV_ASYNC_H
#define KV_ASYNC_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include "dbpool.h"

// Types of async operations
enum class AsyncOpType {
    INSERT_OP,
    DELETE_OP
};

struct AsyncTask {
    AsyncOpType type;
    std::string key;
    std::string value;   // used only for insert
};

class AsyncWriter {
public:
    AsyncWriter(MySQLPool *pool);
    ~AsyncWriter();

    void async_insert(const std::string &key, const std::string &value);
    void async_delete(const std::string &key);

    void start();   // start worker thread
    void stop();    // stop worker thread safely

private:
    void worker_loop();  // worker thread function

    std::queue<AsyncTask> queue_;
    std::mutex mu_;
    std::condition_variable cv_;

    std::thread worker_;
    std::atomic<bool> running_;

    MySQLPool *dbpool_;
};

#endif // KV_ASYNC_H
