#include "db.h"
#include <iostream>
#include <string>
#include <libpq-fe.h>

using namespace std;

PGconn* db_connect(const string& conninfo) {
    PGconn* conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "DB connect failed: " << PQerrorMessage(conn) << endl;
        PQfinish(conn);
        return nullptr;
    }
    return conn;
}

void db_close(PGconn* conn) {
    if (conn) PQfinish(conn);
}

bool db_create(PGconn* conn, const string& key, const string& value) {
    const char* sql =
        "INSERT INTO kv (key, value) VALUES ($1, $2) "
        "ON CONFLICT (key) DO UPDATE SET value = $3;";
    const char* paramValues[3] = { key.c_str(), value.c_str(), value.c_str() };

    PGresult* res = PQexecParams(conn, sql, 3, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "db_create failed: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

string db_read(PGconn* conn, const string& key) {
    const char* sql = "SELECT value FROM kv WHERE key = $1;";
    const char* paramValues[1] = { key.c_str() };

    PGresult* res = PQexecParams(conn, sql, 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return "";
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return "";
    }

    string value = PQgetvalue(res, 0, 0);
    PQclear(res);
    //printf("Value return from Database\n");
    return value;
}

bool db_delete(PGconn* conn, const string& key) {
    const char* sql = "DELETE FROM kv WHERE key = $1;";
    const char* paramValues[1] = { key.c_str() };

    PGresult* res = PQexecParams(conn, sql, 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "db_delete failed: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}
