#include "httplib.h"
#include "db.h"
#include "cache.h"
#include <iostream>
#include <csignal>
#include <mutex>
#include <memory>

using namespace std;
using namespace httplib;

// PostgreSQL connection info
static const string DB_CONNINFO =
    "user=ayan password=Ayan@2003 dbname=kvstore host=127.0.0.1 port=5432";

// Global DB connection + mutex
static PGconn* conn = nullptr;
static mutex db_mutex;

// Handle Ctrl+C (SIGINT)
void handle_sigint(int) {
    cout << "\n Server shutting down..." << endl;
    db_close(conn);
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    // Initialize cache with capacity 5
    cache_init(5);

    // Connect to PostgreSQL
    conn = db_connect(DB_CONNINFO);
    if (!conn) {
        cerr << " Failed to connect to PostgreSQL" << endl;
        return 1;
    }

    Server svr;

    // ---------------- CREATE ----------------
    svr.Post("/create", [](const Request &req, Response &res) {
        //cout <<"Debug created"<<endl;
        //cache_display();
        string key = req.get_param_value("key");
        string value = req.get_param_value("value");

        if (key.empty() || value.empty()) {
            res.status = 400;
            res.set_content("Missing key or value", "text/plain");
            return;
        }

        lock_guard<mutex> lock(db_mutex);
        if (db_create(conn, key, value)) {
            cache_put(key, value);
            res.set_content(" Created successfully\n", "text/plain");
        } else {
            res.status = 500;
            res.set_content("DB insert error\n", "text/plain");
        }
    });

    // ---------------- READ ----------------
    svr.Get("/read", [](const Request &req, Response &res) {
        //cout <<"Debug read"<<endl;
        //cache_display();
        string key = req.get_param_value("key");

        if (key.empty()) {
            res.status = 400;
            res.set_content("Missing key\n", "text/plain");
            return;
        }

        // Try cache first
        string cached = cache_get(key);
        if (!cached.empty()) {
            res.set_content(cached + "\n", "text/plain");
            return;
        }
        

        // If not in cache, fetch from DB
        lock_guard<mutex> lock(db_mutex);
        string value = db_read(conn, key);
        if (!value.empty()) {
            cache_put(key, value);
            res.set_content(value + "\n", "text/plain");
        } else {
            res.status = 404;
            res.set_content("Key not found\n", "text/plain");
        }
    });

    // ---------------- DELETE ----------------
    svr.Delete("/delete", [](const Request &req, Response &res) {
        //cout <<"Debug delete"<<endl;
        //cache_display();
        string key = req.get_param_value("key");

        if (key.empty()) {
            res.status = 400;
            res.set_content("Missing key\n", "text/plain");
            return;
        }

        lock_guard<mutex> lock(db_mutex);
        if (db_delete(conn, key)) {
            cache_delete(key);
            res.set_content(" Deleted successfully\n", "text/plain");
        } else {
            res.status = 500;
            res.set_content("DB delete failed\n", "text/plain");
        }
    });

    cout << "🚀 Server running at http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);

    db_close(conn);
    return 0;
}
