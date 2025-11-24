#ifndef KV_CACHE_H
#define KV_CACHE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <mutex>
#include <memory>

// Simple sharded LRU cache with user-friendly API:
//   cache_get(key, val)
//   cache_put(key, val)
//   cache_delete(key)
//   cache_display()
//   cache_size()

class ShardedLRUCache {
public:
    ShardedLRUCache(size_t num_shards = 32, size_t per_shard_capacity = 256);

    // Returns true if key found, fills value.
    bool cache_get(const std::string &key, std::string &value);

    // Insert/update key-value.
    void cache_put(const std::string &key, const std::string &value);

    // Remove a key.
    void cache_delete(const std::string &key);

    // Print all keys stored (for debugging).
    void cache_display();

    // Approximate total size across all shards.
    size_t cache_size();

private:
    struct Shard {
        std::list<std::string> lru_list; // recent at front
        std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>> map;
        std::mutex mtx;
        size_t capacity;

        Shard(size_t cap) : capacity(cap) {}
    };

    size_t shard_index(const std::string &key) const;

    size_t num_shards_;
    size_t per_shard_capacity_;
    std::vector<std::unique_ptr<Shard>> shards_;
};

#endif // KV_CACHE_H
