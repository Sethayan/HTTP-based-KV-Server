#ifndef DB_H
#define DB_H

#include <libpq-fe.h>
#include <string>

using namespace std;

PGconn* db_connect(const string& conninfo);
void db_close(PGconn* conn);

bool db_create(PGconn* conn, const string& key, const string& value);
string db_read(PGconn* conn, const string& key);
bool db_delete(PGconn* conn, const string& key);

#endif
