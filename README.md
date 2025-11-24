
# HTTP-Based Key-Value Store  

A lightweight, high-performance HTTP Key–Value Storage Server built using C++, CivetWeb, threading, and an optional in-memory LRU cache.
The server exposes simple HTTP endpoints (PUT, GET, DELETE) that allow clients to store, retrieve, and delete key–value pairs over HTTP.

# Project Description

A lightweight, high-performance C++–based Key-Value Storage Server built using g++, MySQL libpq, threading, and a  in-memory LRU  cache layer.
The server exposes simple HTTP endpoints (PUT, GET, DELETE) that allow clients to store, retrieve, and remove key–value pairs over HTTP.

# Feature 
1. CivetWeb-based HTTP Server (server.cpp, CivetServer.cpp)

2. MySQL/PostgreSQL-backed storage (dbpool.cpp)

3. High-speed sharded LRU cache (cache.cpp)

4. Async writer thread for DB persistence (async.cpp)

5. Multi-threaded request handling

6. Simple Makefile for easy compilation

7. Supports GET / PUT / DELETE over HTTP


##  Installation Procedure

Follow these steps to set up and run the project locally:

1. Clone this repository or download the source code.
2. Navigate to the project directory.

**Run The Server**
1. Navigate to Server directory
2. Run the MakeFile
   ```bash
   make run
   
**Run The Client**
1. Navigate to Client directory
```bash
taskset -c 2-5 ./loadgen <No of Thread> <duration> <Workload>
  


 

