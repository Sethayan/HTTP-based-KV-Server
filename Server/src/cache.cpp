#include "cache.h"
#include <iostream>
#include <functional>

ShardedLRUCache::ShardedLRUCache(size_t num_shards, size_t per_shard_capacity)
    : num_shards_(num_shards), per_shard_capacity_(per_shard_capacity)
{
    shards_.reserve(num_shards_);
    for (size_t i = 0; i < num_shards; i++) {
        shards_.push_back(std::make_unique<Shard>(per_shard_capacity));
    }
}

size_t ShardedLRUCache::shard_index(const std::string &key) const {
    return std::hash<std::string>{}(key) % num_shards_;
}


bool ShardedLRUCache::cache_get(const std::string &key, std::string &value) {
    size_t s = shard_index(key);
    Shard *sh = shards_[s].get();
    std::lock_guard<std::mutex> lk(sh->mtx);

    auto it = sh->map.find(key);
    if (it == sh->map.end())
        return false;

    // Move key to front (most recently used)
    sh->lru_list.erase(it->second.second);
    sh->lru_list.push_front(key);
    it->second.second = sh->lru_list.begin();

    value = it->second.first;
    return true;
}


void ShardedLRUCache::cache_put(const std::string &key, const std::string &value) {
    size_t s = shard_index(key);
    Shard *sh = shards_[s].get();
    std::lock_guard<std::mutex> lk(sh->mtx);

    auto it = sh->map.find(key);
    if (it != sh->map.end()) {
        // update existing
        sh->lru_list.erase(it->second.second);
        sh->lru_list.push_front(key);
        it->second = {value, sh->lru_list.begin()};
        return;
    }

    // evict if full
    if (sh->map.size() >= sh->capacity) {
        std::string old = sh->lru_list.back();
        sh->lru_list.pop_back();
        sh->map.erase(old);
    }

    // insert
    sh->lru_list.push_front(key);
    sh->map.emplace(key, std::make_pair(value, sh->lru_list.begin()));
}


void ShardedLRUCache::cache_delete(const std::string &key) {
    size_t s = shard_index(key);
    Shard *sh = shards_[s].get();
    std::lock_guard<std::mutex> lk(sh->mtx);

    auto it = sh->map.find(key);
    if (it == sh->map.end()) return;

    sh->lru_list.erase(it->second.second);
    sh->map.erase(it);
}


void ShardedLRUCache::cache_display() {
    for (size_t s = 0; s < num_shards_; s++) {
        Shard *sh = shards_[s].get();
        std::lock_guard<std::mutex> lk(sh->mtx);

        std::cout << "Shard " << s << " (" << sh->map.size() << " items): ";
        for (auto &k : sh->lru_list)
            std::cout << k << "  ";
        std::cout << "\n";
    }
}


size_t ShardedLRUCache::cache_size() {
    size_t total = 0;
    for (size_t s = 0; s < num_shards_; s++) {
        Shard *sh = shards_[s].get();
        std::lock_guard<std::mutex> lk(sh->mtx);
        total += sh->map.size();
    }
    return total;
}
