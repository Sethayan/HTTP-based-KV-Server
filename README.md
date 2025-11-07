
# HTTP-Based Key-Value Store  

A lightweight HTTP-based Key–Value storage server built in C++, using PostgreSQL for persistence and an optional in-memory cache for fast access.

# Project Description

A lightweight, high-performance C++–based Key-Value Storage Server built using g++, PostgreSQL libpq, threading, and a  in-memory LRU  cache layer.
The server exposes simple HTTP endpoints (PUT, GET, DELETE) that allow clients to store, retrieve, and remove key–value pairs over HTTP.

# Feature 

1.Custom C++ Server (server.cpp)
2.PostgreSQL-based key-value store (db.cpp)
3.LRU Cache system (cache.cpp)
4.Multi-threaded request processing using POSIX threads
5.Easy to build with a simple Makefile

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
1. Open another terminal 
2. HTTP Request for create:
    ```bash
    curl -X POST -d "key=user1&value=myvalue" http://127.0.0.1:8080/create
3. HTTP Request for read
    ```bash
   curl -X GET "http://127.0.0.1:8080/read?key=user1"
4. HTTP Request for delete
   ```bash
   curl -X DELETE "http://127.0.0.1:8080/delete?key=user1"
  


 

