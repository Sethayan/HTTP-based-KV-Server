#include "cache.h"
#include <iostream>
#include <unordered_map>
#include <list>
#include <string>

using namespace std;

// LRUCache class instance
static LRUCache cache;

void cache_init(int capacity) {
    cache.capacity = capacity;
    cache.items.clear();
    cache.order.clear();
}

string cache_get(const string &key) {
    auto it = cache.items.find(key);
    if (it == cache.items.end()) {
        return "";  // not found
    }

    // Move this key to the front (most recently used)
    cache.order.splice(cache.order.begin(), cache.order, it->second.second);
    //printf("Value return from cache\n");
    return it->second.first;
}

void cache_put(const string &key, const string &value) {
    auto it = cache.items.find(key);

    if (it != cache.items.end()) {
        // Update existing value
        it->second.first = value;
        // Move to front (recently used)
        cache.order.splice(cache.order.begin(), cache.order, it->second.second);
        return;
    }

    // If full, remove least recently used
    if (cache.items.size() >= cache.capacity) {
        string lru_key = cache.order.back();
        cache.order.pop_back();
        cache.items.erase(lru_key);
    }

    // Insert new key
    cache.order.push_front(key);
    cache.items[key] = {value, cache.order.begin()};
}

void cache_delete(const string &key) {
    auto it = cache.items.find(key);
    if (it != cache.items.end()) {
        cache.order.erase(it->second.second);
        cache.items.erase(it);
    }
}

void cache_display() {
    cout << "Cache (MRU → LRU): ";
    for (const auto &k : cache.order) {
        cout << "(" << k << " → " << cache.items[k].first << ") ";
    }
    cout << endl;
}
