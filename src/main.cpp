#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
#include <git2.h>

#include "auth.hpp"
#include "clone.hpp"
#include "analyze.hpp"
#include "utils.hpp"
#include "globals.hpp"
#include "print_sorted.hpp" 

namespace fs = std::filesystem;
using json = nlohmann::json;

int main() {
    git_libgit2_init();

    std::string token_or_username;
    bool using_token = false;
    json repos = promptAndFetchRepos(token_or_username, using_token);
    
    // Start measuring time after fetching repos
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    fs::path temp_base = fs::temp_directory_path();
    fs::path base_clone_dir = temp_base / ("github_clones_" + generateRandomSuffix());
    fs::create_directories(base_clone_dir);

    for (json::const_iterator it = repos.begin(); it != repos.end(); ++it) {
        const json& repo = *it;
        if (!repo["fork"].get<bool>()) {
            cloneQueue.push(repo);
        }
    }

    unsigned int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> clonePool;
    std::vector<std::thread> analyzePool;

    for (unsigned int i = 0; i < threads; ++i) {
        std::thread t(analyzeWorker);
        analyzePool.push_back(std::move(t));
    }

    for (unsigned int i = 0; i < threads; ++i) {
        std::thread t(cloneWorker, std::ref(base_clone_dir), std::ref(token_or_username), using_token);
        clonePool.push_back(std::move(t));
    }

    for (std::vector<std::thread>::iterator it = clonePool.begin(); it != clonePool.end(); ++it) {
        it->join();
    }

    {
        std::lock_guard<std::mutex> lock(analyze_mutex);
        cloning_done = true;
        analyze_cv.notify_all();
    }

    for (std::vector<std::thread>::iterator it = analyzePool.begin(); it != analyzePool.end(); ++it) {
        it->join();
    }

    // NEW: Sorted language stats output
    printSortedLanguageStats(lang_totals);

    std::error_code ec;
    fs::remove_all(base_clone_dir, ec);
    if (ec) {
        std::cerr << "Failed to delete temp dir: " << ec.message() << "\n";
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "\nTotal runtime (post-auth to exit): " << elapsed.count() << " seconds\n";

    git_libgit2_shutdown();
    return 0;
}
