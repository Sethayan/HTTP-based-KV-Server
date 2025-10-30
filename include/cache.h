#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <list>
#include <string>
using namespace std;

struct LRUCache {
    int capacity;
    list<string> order;  // MRU at front, LRU at back
    unordered_map<string, pair<string, list<string>::iterator>> items;
};

void cache_init(int capacity = 5);
string cache_get(const string &key);
void cache_put(const string &key, const string &value);
void cache_delete(const string &key);
void cache_display();

#endif
