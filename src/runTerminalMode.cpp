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
#include "pureOAuthServer.hpp"  // For waitForOAuthCode()

namespace fs = std::filesystem;
using json = nlohmann::json;

int runTerminalMode() {
    git_libgit2_init();

    while (true) {
        {
            std::lock_guard<std::mutex> lock(analyze_mutex);
            while (!analyzeQueue.empty()) analyzeQueue.pop();
            while (!cloneQueue.empty()) cloneQueue.pop();
            lang_totals.clear();
            cloning_done = false;
        }

        std::string token_or_username;
        bool using_token = false;

        std::cout << "\nLogin Options:\n";
        std::cout << "  1. GitHub Token or Username\n";
        std::cout << "  2. Login with GitHub via OAuth\n";
        std::cout << "Enter 1 or 2: ";
        std::string option;
        std::getline(std::cin, option);

        if (option == "2") {
            std::cout << "Opening browser for GitHub login...\n";

            std::string clientId = "Ov23liVQ3EBVLbcP681k";
            std::string url = "https://github.com/login/oauth/authorize?client_id=" + clientId +
                  "&scope=repo&redirect_uri=http://localhost:8000/callback";


#ifdef __APPLE__
            std::string openCmd = "open \"" + url + "\"";
#elif __linux__
            std::string openCmd = "xdg-open \"" + url + "\"";
#elif _WIN32
            std::string openCmd = "start \"\" \"" + url + "\"";
#else
#error "Unsupported platform for launching browser."
#endif
            system(openCmd.c_str());

            std::string code = waitForOAuthCode();
            if (code.empty()) {
                std::cerr << "Failed to get authorization code.\n";
                return 1;
            }

            token_or_username = exchangeCodeForToken(code);
            if (token_or_username.empty()) {
                std::cerr << "Failed to exchange code for token.\n";
                return 1;
            }

            using_token = true;

        } else {
            std::cout << "Enter GitHub token or username: ";
            std::getline(std::cin, token_or_username);
            using_token = (token_or_username.length() >= 40);
        }

        json repos = promptAndFetchRepos(token_or_username, using_token);

        auto start = std::chrono::steady_clock::now();

        fs::path temp = fs::temp_directory_path();
        fs::path base = temp / ("github_clones_" + generateRandomSuffix());
        fs::create_directories(base);

        for (const json& repo : repos) {
            if (!repo["fork"].get<bool>())
                cloneQueue.push(repo);
        }

        unsigned int threads = std::thread::hardware_concurrency();
        std::vector<std::thread> clonePool, analyzePool;

        for (unsigned int i = 0; i < threads; ++i)
            analyzePool.emplace_back(analyzeWorker);

        for (unsigned int i = 0; i < threads; ++i)
            clonePool.emplace_back(cloneWorker, std::ref(base), std::ref(token_or_username), using_token);

        for (auto& t : clonePool) t.join();

        {
            std::lock_guard<std::mutex> lock(analyze_mutex);
            cloning_done = true;
            analyze_cv.notify_all();
        }

        for (auto& t : analyzePool) t.join();

        printSortedLanguageStats(lang_totals);

        std::error_code ec;
        fs::remove_all(base, ec);
        if (ec)
            std::cerr << "Cleanup failed: " << ec.message() << "\n";

        auto end = std::chrono::steady_clock::now();
        std::cout << "\nTotal runtime: " << std::chrono::duration<double>(end - start).count() << " seconds\n";

        std::string again;
        std::cout << "\nRun again? [y/n]: ";
        std::getline(std::cin, again);
        if (again != "y" && again != "Y") break;
    }

    git_libgit2_shutdown();
    return 0;
}
