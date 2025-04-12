#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <git2.h>

#include "auth.hpp"
#include "clone.hpp"
#include "analyze.hpp"
#include "utils.hpp"
#include "globals.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

int main() {
    git_libgit2_init();

    std::string token_or_username;
    bool using_token = false;
    json repos = promptAndFetchRepos(token_or_username, using_token);

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
        analyzePool.push_back(std::thread(analyzeWorker));
    }

    for (unsigned int i = 0; i < threads; ++i) {
        clonePool.push_back(std::thread(cloneWorker, std::ref(base_clone_dir), std::ref(token_or_username), using_token));
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

    int total = 0;
    for (std::unordered_map<std::string, int>::const_iterator it = lang_totals.begin(); it != lang_totals.end(); ++it) {
        total += it->second;
    }

    std::cout << "\nLanguage Breakdown (by lines of code):\n";
    for (std::unordered_map<std::string, int>::const_iterator it = lang_totals.begin(); it != lang_totals.end(); ++it) {
        const std::string& lang = it->first;
        int lines = it->second;
        printf("%-20s : %6.2f%% (%d lines)\n", lang.c_str(), (100.0 * lines) / total, lines);
    }

    std::error_code ec;
    fs::remove_all(base_clone_dir, ec);
    if (ec) {
        std::cerr << "Failed to delete temp dir: " << ec.message() << "\n";
    }

    git_libgit2_shutdown();
    return 0;
}
