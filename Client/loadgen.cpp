// loadgen.cpp — rewritten for your KV server on port 8080
// Supports workloads: get-popular, get-all, put-all, get-put
// Endpoints:
//   POST   /create   (key=value form)
//   GET    /get?key=
//   DELETE /delete?key=

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <mutex>
#include <curl/curl.h>

std::atomic<long long> total_requests(0);
std::atomic<long long> total_response_time_us(0);
std::atomic<long long> total_failed(0);
std::atomic<bool> stop_test(false);

const std::string BASE_URL = "http://127.0.0.1:8080";

std::mutex pop_mtx;
std::vector<std::string> popular_keys;
const int POPULAR_KEY_COUNT = 50;

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

// -----------------------------------------------------
// Request helpers
// -----------------------------------------------------

bool do_get(CURL *curl, const std::string &url) {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    long code = 0;
    CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    return (rc == CURLE_OK && code == 200);
}

bool do_delete(CURL *curl, const std::string &url) {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    long code = 0;
    CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
    return (rc == CURLE_OK && code == 200);
}

bool do_post(CURL *curl, const std::string &key, const std::string &val) {
    std::string body = "key=" + key + "&value=" + val;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, (BASE_URL + "/create").c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    long code = 0;
    CURLcode rc = curl_easy_perform(curl);

    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);

    curl_slist_free_all(headers);

    return (rc == CURLE_OK && code == 200);
}

// -----------------------------------------------------
// Worker thread
// -----------------------------------------------------

void worker(const std::string &workload) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 500L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> psel(0, POPULAR_KEY_COUNT - 1);
    std::uniform_int_distribution<> mode(0, 99);

    while (!stop_test.load()) {
        auto t1 = std::chrono::steady_clock::now();
        bool ok = false;

        if (workload == "put-all") {
            std::string key = "k" + std::to_string(rng() % 1000000);
            std::string val = "v" + std::to_string(rng());
            ok = do_post(curl, key, val);

        } else if (workload == "get-all") {
            std::string key = "k" + std::to_string(rng() % 1000000);
            ok = do_get(curl, BASE_URL + "/get?key=" + key);

        } else if (workload == "get-popular") {
            std::string key;
            {
                std::lock_guard<std::mutex> lk(pop_mtx);
                key = popular_keys[psel(rng)];
            }
            ok = do_get(curl, BASE_URL + "/get?key=" + key);

        } else if (workload == "get-put") {

            int r = mode(rng);

            if (r < 70) {
                // GET popular
                std::string key;
                {
                    std::lock_guard<std::mutex> lk(pop_mtx);
                    key = popular_keys[psel(rng)];
                }
                ok = do_get(curl, BASE_URL + "/get?key=" + key);

            } else if (r < 90) {
                // random PUT
                std::string key = "k" + std::to_string(rng() % 1000000);
                std::string val = "v" + std::to_string(rng());
                ok = do_post(curl, key, val);

            } else {
                // random DELETE
                std::string key = "k" + std::to_string(rng() % 1000000);
                ok = do_delete(curl, BASE_URL + "/delete?key=" + key);
            }
        }

        if (ok) {
            auto t2 = std::chrono::steady_clock::now();
            long long us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            total_requests++;
            total_response_time_us += us;
        } else {
            total_failed++;
        }
    }

    curl_easy_cleanup(curl);
}

// -----------------------------------------------------
// Pre-populate popular keys (for get-popular, get-put)
// -----------------------------------------------------

void prepopulate() {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < POPULAR_KEY_COUNT; i++) {
        std::string key = "popular_" + std::to_string(i);
        std::string val = "val_" + std::to_string(rng());

        if (do_post(curl, key, val)) {
            popular_keys.push_back(key);
        }
    }

    curl_easy_cleanup(curl);

    if (popular_keys.empty()) {
        std::cerr << "ERROR: Prepopulate failed. Server not responding?\n";
        exit(1);
    }
}

// -----------------------------------------------------
// Main
// -----------------------------------------------------

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cout << "Usage: ./loadgen <threads> <duration> <workload>\n";
        std::cout << "Workloads: get-popular | get-all | put-all | get-put\n";
        return 1;
    }

    int threads = std::stoi(argv[1]);
    int duration = std::stoi(argv[2]);
    std::string workload = argv[3];

    curl_global_init(CURL_GLOBAL_ALL);

    if (workload == "get-popular" || workload == "get-put") {
        prepopulate();
    }

    std::vector<std::thread> pool;
    for (int i = 0; i < threads; i++)
        pool.emplace_back(worker, workload);

    std::this_thread::sleep_for(std::chrono::seconds(duration));
    stop_test = true;

    for (auto &t : pool) t.join();

    long long ok = total_requests.load();
    long long failed = total_failed.load();

    double tps = ok / (double)duration;
    double avg_us = ok ? (double)total_response_time_us.load() / ok : 0;

    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Threads:      " << threads << "\n";
    std::cout << "Duration:     " << duration << " sec\n";
    std::cout << "Success:      " << ok << "\n";
    std::cout << "Failed:       " << failed << "\n";
    std::cout << "Throughput:   " << tps << " req/s\n";
    std::cout << "Avg Latency:  " << avg_us << " us\n";
    std::cout << "================\n";

    curl_global_cleanup();
    return 0;
}
