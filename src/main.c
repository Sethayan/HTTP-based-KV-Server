#include <stdio.h>
#include "db.h"

int main() {
    PGconn *conn = connect_db();

    // Insert some data
    insert_key_value(conn, "Dbname", "kv");
    insert_key_value(conn, "project", "KVServer");

    // Display everything
    fetch_all(conn);

    PQfinish(conn);
    return 0;
}
