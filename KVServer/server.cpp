#include "cpp-httplib/httplib.h"
#include <iostream>
using namespace httplib;

int main() {
    Server svr;

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello from C++ server!", "text/plain");
    });

    std::cout << "Server started on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
}
