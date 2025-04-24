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

int runTerminalMode() {
    git_libgit2_init();

    while (true) {
        // Reset shared globals before each run
        {
            std::lock_guard<std::mutex> lock(analyze_mutex);
            while (!analyzeQueue.empty()) analyzeQueue.pop();
            while (!cloneQueue.empty()) cloneQueue.pop();
            lang_totals.clear();
            cloning_done = false;
        }

        std::string token_or_username;
        bool using_token = false;
        std::cout << "Enter GitHub token or username: ";
        std::getline(std::cin, token_or_username);

        using_token = (token_or_username.length() == 40);
        json repos = promptAndFetchRepos(token_or_username, using_token);

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();


        fs::path temp_base = fs::temp_directory_path();
        fs::path base_clone_dir = temp_base / ("github_clones_" + generateRandomSuffix());
        fs::create_directories(base_clone_dir);

        for (const json& repo : repos) {
            if (!repo["fork"].get<bool>()) {
                cloneQueue.push(repo);
            }
        }

        unsigned int threads = std::thread::hardware_concurrency();
        std::vector<std::thread> clonePool;
        std::vector<std::thread> analyzePool;

        for (unsigned int i = 0; i < threads; ++i)
            analyzePool.emplace_back(analyzeWorker);

        for (unsigned int i = 0; i < threads; ++i)
            clonePool.emplace_back(cloneWorker, std::ref(base_clone_dir), std::ref(token_or_username), using_token);

        for (std::thread& t : clonePool) t.join();

        {
            std::lock_guard<std::mutex> lock(analyze_mutex);
            cloning_done = true;
            analyze_cv.notify_all();
        }

        for (std::thread& t : analyzePool) t.join();

        printSortedLanguageStats(lang_totals);

        std::error_code ec;
        fs::remove_all(base_clone_dir, ec);
        if (ec) {
            std::cerr << "Failed to delete temp dir: " << ec.message() << "\n";
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "\nTotal runtime: " << elapsed.count() << " seconds\n";

        std::string again;
        std::cout << "\nWould you like to input another username or token? [y/n]: ";
        std::getline(std::cin, again);

        if (again != "y" && again != "Y") break;
    }

    git_libgit2_shutdown();
    return 0;
}
