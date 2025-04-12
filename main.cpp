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

    for (const auto& repo : repos)
        if (!repo["fork"].get<bool>())
            cloneQueue.push(repo);

    unsigned int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> clonePool, analyzePool;

    for (unsigned int i = 0; i < threads; ++i)
        analyzePool.emplace_back(analyzeWorker);

    for (unsigned int i = 0; i < threads; ++i)
        clonePool.emplace_back(cloneWorker, std::ref(base_clone_dir), std::ref(token_or_username), using_token);

    for (auto& t : clonePool) t.join();

    {
        std::lock_guard<std::mutex> lock(analyze_mutex);
        cloning_done = true;
        analyze_cv.notify_all();
    }

    for (auto& t : analyzePool) t.join();

    int total = 0;
    for (const auto& [_, lines] : lang_totals)
        total += lines;

    std::cout << "\nLanguage Breakdown (by lines of code):\n";
    for (const auto& [lang, lines] : lang_totals)
        printf("%-20s : %6.2f%% (%d lines)\n", lang.c_str(), (100.0 * lines) / total, lines);

    std::error_code ec;
    fs::remove_all(base_clone_dir, ec);
    if (ec)
        std::cerr << "Failed to delete temp dir: " << ec.message() << "\n";

    git_libgit2_shutdown();
    return 0;
}
