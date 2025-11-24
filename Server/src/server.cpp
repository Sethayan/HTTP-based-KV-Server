#include "CivetServer.h"
#include "cache.h"
#include "dbpool.h"
#include "async.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

// ---------------------------
// GLOBAL OBJECTS
// ---------------------------
ShardedLRUCache cache(32, 256);
MySQLPool *dbpool = nullptr;
AsyncWriter *asyncWriter = nullptr;

// Escape helper
static std::string sql_escape(MYSQL *conn, const std::string &s) {
    std::string out;
    out.resize(s.size() * 2 + 1);
    unsigned long n = mysql_real_escape_string(conn, &out[0],
                                               s.c_str(), s.size());
    out.resize(n);
    return out;
}

// ========================================================
//                   HANDLER CLASS
// ========================================================
class KVHandler : public CivetHandler {

public:

// ---------------------- GET ----------------------------
bool handleGet(CivetServer *, mg_connection *conn) override {

    const mg_request_info *ri = mg_get_request_info(conn);

    char keybuf[512] = {0};
    if (ri->query_string) {
        mg_get_var(ri->query_string, strlen(ri->query_string),
                   "key", keybuf, sizeof(keybuf));
    }

    std::string key = keybuf;

    if (key.empty()) {
        mg_printf(conn,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n\r\nmissing key\n");
        return true;
    }

    // 1. Try cache
    std::string value;
    if (cache.cache_get(key, value)) {
        mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s",
            value.c_str());
        return true;
    }

    // 2. DB read
    MYSQL *c = dbpool->acquire();

    std::string ek = sql_escape(c, key);
    std::string q = "SELECT v FROM kvstore WHERE k='" + ek + "'";

    if (mysql_query(c, q.c_str())) {
        std::string err = mysql_error(c);
        dbpool->release(c);

        mg_printf(conn,
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n\r\nDB error: %s\n", err.c_str());
        return true;
    }

    MYSQL_RES *res = mysql_store_result(c);
    bool found = false;

    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row) {
            value = row[0] ? row[0] : "";
            found = true;
        }
        mysql_free_result(res);
    }

    dbpool->release(c);

    if (!found) {
        mg_printf(conn,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n\r\nnot found\n");
        return true;
    }

    // store to cache
    cache.cache_put(key, value);

    mg_printf(conn,
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s",
        value.c_str());
    return true;
}



// ---------------------- CREATE (POST) --------------------
bool handlePost(CivetServer *, mg_connection *conn) override {

    const mg_request_info *ri = mg_get_request_info(conn);

    long long len = ri->content_length;
    std::string body;

    if (len > 0) {
        body.resize(len);
        mg_read(conn, &body[0], len);
    }

    char kbuf[512] = {0};
    char vbuf[4096] = {0};

    mg_get_var(body.c_str(), body.size(), "key", kbuf, sizeof(kbuf));
    mg_get_var(body.c_str(), body.size(), "value", vbuf, sizeof(vbuf));

    std::string key = kbuf;
    std::string value = vbuf;

    if (key.empty()) {
        mg_printf(conn,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n\r\nmissing key\n");
        return true;
    }

    // Update cache
    cache.cache_put(key, value);

    // Async DB insert/update
    asyncWriter->async_insert(key, value);

    mg_printf(conn,
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nok\n");
    return true;
}



// ---------------------- DELETE ---------------------------
bool handleDelete(CivetServer *, mg_connection *conn) override {

    const mg_request_info *ri = mg_get_request_info(conn);

    char keybuf[512] = {0};
    if (ri->query_string) {
        mg_get_var(ri->query_string, strlen(ri->query_string),
                   "key", keybuf, sizeof(keybuf));
    }

    std::string key = keybuf;

    if (key.empty()) {
        mg_printf(conn,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n\r\nmissing key\n");
        return true;
    }

    // remove from cache
    cache.cache_delete(key);

    // async delete from db
    asyncWriter->async_delete(key);

    mg_printf(conn,
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\ndeleted\n");
    return true;
}

}; // END OF CLASS



// ========================================================
//                         MAIN
// ========================================================
int main() {

    try {
        dbpool = new MySQLPool(
            "127.0.0.1",
            "root",
            "Ayan@2003",
            "kvdb",
            3306,
            8
        );

        asyncWriter = new AsyncWriter(dbpool);
        asyncWriter->start();
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    const char *opts[] = {
        "listening_ports", "8080",
        nullptr
    };

    CivetServer server(opts);

    KVHandler handler;

    server.addHandler("/create", handler);
    server.addHandler("/get", handler);
    server.addHandler("/delete", handler);

    std::cout << "KV Server running on port 8080\n";
    getchar();

    asyncWriter->stop();
    delete asyncWriter;
    delete dbpool;

    return 0;
}
