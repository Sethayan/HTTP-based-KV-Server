
# HTTP-Based Key-Value Store  

A lightweight HTTP-based Key–Value storage server built in C++, using PostgreSQL for persistence and an optional in-memory cache for fast access.
The server exposes simple HTTP endpoints (PUT, GET, DELETE) that allow clients to store, retrieve, and remove key–value pairs over HTTP.

It also includes a multithreaded load generator for stress testing the server.

