// tiny_server.cpp
#include "CivetServer.h"
#include <string>

class KeyHandler : public CivetHandler {
public:
  bool handleGet(CivetServer *server, struct mg_connection *conn) override {
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello from KV server\n");
    return true;
  }
};

int main() {
  const char *options[] = {"listening_ports", "8888", nullptr};
  CivetServer server(options);
  KeyHandler h;
  server.addHandler("/key", h);
  printf("listening on 8888\n");
  getchar(); // keep running
  return 0;
}
