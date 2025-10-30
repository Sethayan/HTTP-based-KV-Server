// loadgen.cpp
// Compile: g++ -std=c++17 loadgen.cpp -lcurl -lpthread -O2 -o loadgen

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <cstring>
#include <curl/curl.h>

using namespace std;
using namespace std::chrono;

// ---------- Globals / Stats ----------
atomic<long long> total_latency_us(0); // accumulated latency in microseconds
atomic<long long> successful_requests(0);
atomic<long long> failed_requests(0);
atomic<bool> stop_flag(false);

// ---------- Config / Workload ----------
enum class Workload { PUT_ALL, GET_ALL, GET_POPULAR, MIXED };

struct Config {
    string host = "127.0.0.1";
    int port = 8080;
    int threads = 4;
    int duration = 30; // seconds
    Workload workload = Workload::MIXED;
    int popular_count = 10; // for GET_POPULAR
    int key_space_per_thread = 100; // used for unique keys
};

// write callback that discards response body
static size_t discard_write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    (void)ptr; (void)userdata;
    return size * nmemb;
}

// build URL string helper
static string url_base(const Config &cfg) {
    return string("http://") + cfg.host + ":" + to_string(cfg.port);
}

// preload popular keys (synchronous, single-threaded)
void preload_popular(const Config &cfg) {
    string base = url_base(cfg);
    CURL *curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_write_cb);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

    for (int i = 0; i < cfg.popular_count; ++i) {
        string post_data = "key=popular_" + to_string(i) + "&value=popular_val_" + to_string(i);
        string url = base + "/create";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_perform(curl);
    }

    curl_easy_cleanup(curl);
}


// thread-local random helpers
inline int randint(mt19937 &rng, int lo, int hi) {
    return uniform_int_distribution<int>(lo, hi)(rng);
}

// create/read/delete request generator per workload
void client_thread(int id, const Config &cfg) {
    string base = url_base(cfg);
    CURL *curl = curl_easy_init();
    if (!curl) {
        cerr << "Thread " << id << ": curl init failed\n";
        return;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_write_cb);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

    // RNG per thread
    random_device rd;
    mt19937 rng(rd() ^ (id << 16));

    long long local_counter = 0;
        while (!stop_flag.load(memory_order_relaxed)) {
        string url;
        const char *custom_method = nullptr;
        string post_data;

        if (cfg.workload == Workload::PUT_ALL) {
            // POST with body data instead of query
            url = base + "/create";
            post_data = "key=put_t" + to_string(id) + "_" + to_string(local_counter) +
                        "&value=" + to_string(randint(rng, 0, 1000000));
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            custom_method = nullptr;
        }
        else if (cfg.workload == Workload::GET_ALL) {
            url = base + "/read?key=getall_t" + to_string(id) + "_" + to_string(local_counter);
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            custom_method = nullptr;
        }
        else if (cfg.workload == Workload::GET_POPULAR) {
            int k = randint(rng, 0, max(1, cfg.popular_count) - 1);
            url = base + "/read?key=popular_" + to_string(k);
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            custom_method = nullptr;
        }
        else { // MIXED workload
            int op = randint(rng, 0, 99);
            if (op < 40) { // 40% read popular
                int k = randint(rng, 0, max(1, cfg.popular_count) - 1);
                url = base + "/read?key=popular_" + to_string(k);
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                custom_method = nullptr;
            } else if (op < 75) { // 35% create
                url = base + "/create";
                post_data = "key=mixed_t" + to_string(id) + "_" + to_string(local_counter) +
                            "&value=" + to_string(randint(rng, 0, 1000000));
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                custom_method = nullptr;
            } else { // 25% delete
                int k = randint(rng, 0, cfg.key_space_per_thread - 1);
                url = base + "/delete?key=mixed_t" + to_string(id) + "_" + to_string(k);
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                custom_method = "DELETE";
            }
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // ensure we don't accidentally keep previous POST flag if not using POST
        if (cfg.workload != Workload::PUT_ALL && !(cfg.workload == Workload::MIXED && custom_method == nullptr && url.find("/create?") != string::npos)) {
            // best effort clear: set to HTTPGET (curl will override for POST/CUSTOM)
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }

        auto t0 = high_resolution_clock::now();
        CURLcode cres = curl_easy_perform(curl);
        auto t1 = high_resolution_clock::now();

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        long long latency_us = duration_cast<microseconds>(t1 - t0).count();

        if (cres == CURLE_OK && http_code >= 200 && http_code < 300) {
            successful_requests.fetch_add(1, memory_order_relaxed);
            total_latency_us.fetch_add(latency_us, memory_order_relaxed);
        } else {
            failed_requests.fetch_add(1, memory_order_relaxed);
        }

        // reset POST/CUSTOM flags so next iteration is clean
        curl_easy_setopt(curl, CURLOPT_POST, 0L);
        if (custom_method) curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);

        local_counter++;
    }

    curl_easy_cleanup(curl);
}

// ---------- Main ----------
int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0] << " <server_host> <port> <num_threads> <duration_sec> <workload> [popular_count]\n";
        cerr << "workload: put_all | get_all | get_popular | mixed\n";
        cerr << "Example: ./loadgen 127.0.0.1 8080 8 30 mixed 10\n";
        return 1;
    }

    Config cfg;
    cfg.host = argv[1];
    cfg.port = atoi(argv[2]);
    cfg.threads = atoi(argv[3]);
    cfg.duration = atoi(argv[4]);
    string wl = argv[5];
    if (wl == "put_all") cfg.workload = Workload::PUT_ALL;
    else if (wl == "get_all") cfg.workload = Workload::GET_ALL;
    else if (wl == "get_popular") cfg.workload = Workload::GET_POPULAR;
    else if (wl == "mixed") cfg.workload = Workload::MIXED;
    else {
        cerr << "Unknown workload: " << wl << "\n";
        return 1;
    }
    if (argc >= 7) cfg.popular_count = atoi(argv[6]);

    // initialize curl globally
    CURLcode g = curl_global_init(CURL_GLOBAL_ALL);
    if (g != CURLE_OK) {
        cerr << "curl_global_init failed\n";
        return 1;
    }

    // preload popular keys if needed
    if (cfg.workload == Workload::GET_POPULAR || cfg.workload == Workload::MIXED) {
        cout << "[info] Preloading " << cfg.popular_count << " popular keys...\n";
        preload_popular(cfg);
    }

    cout << "[info] Starting closed-loop load test\n";
    cout << " host=" << cfg.host << " port=" << cfg.port
         << " threads=" << cfg.threads << " duration=" << cfg.duration
         << " workload=" << wl << " popular_count=" << cfg.popular_count << "\n";

    // launch threads
    vector<thread> threads;
    threads.reserve(cfg.threads);
    for (int i = 0; i < cfg.threads; ++i) {
        threads.emplace_back(client_thread, i, cfg);
    }

    // sleep for test duration
    this_thread::sleep_for(seconds(cfg.duration));
    stop_flag.store(true, memory_order_relaxed);

    // join threads
    for (auto &t : threads) t.join();

    // compute results
    long long succ = successful_requests.load();
    long long fail = failed_requests.load();
    double duration_s = (double)cfg.duration;
    double throughput = (duration_s > 0) ? (double)succ / duration_s : 0.0;
    double avg_resp_ms = (succ > 0) ? ((double)total_latency_us.load() / 1000.0 / (double)succ) : 0.0;

    cout << "\n=== Load Test Results ===\n";
    cout << "Duration (s):        " << duration_s << "\n";
    cout << "Successful requests: " << succ << "\n";
    cout << "Failed requests:     " << fail << "\n";
    cout << "Throughput (req/s):  " << throughput << "\n";
    cout << "Avg response time:   " << avg_resp_ms << " ms\n";
    cout << "========================\n";

    curl_global_cleanup();
    return 0;
}
