#include "worker.hpp"
#include "auth.hpp"
#include "clone.hpp"
#include "analyze.hpp"
#include "globals.hpp"
#include "print_sorted.hpp"
#include "utils.hpp"

#include <chrono>
#include <thread>
#include <vector>
#include <sstream>
#include <filesystem>
#include <QObject>
#include <git2.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

Worker::Worker(QString input, QTextEdit* outputArea)
    : input(input), outputArea(outputArea) {}

void Worker::run() {
    std::string token_or_username = input.toStdString();
    bool using_token = token_or_username.length() > 30;
    emit logMessage("Fetching repositories...");

    json repos = promptAndFetchRepos(token_or_username, using_token);
    std::ostringstream outStream;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock1(analyze_mutex);
        std::lock_guard<std::mutex> lock2(cout_mutex);
        std::lock_guard<std::mutex> lock3(lang_mutex);
        lang_totals.clear();
        while (!analyzeQueue.empty()) analyzeQueue.pop();
        while (!cloneQueue.empty()) cloneQueue.pop();
        cloning_done = false;

        // Enable GUI logging during analysis
        logCallback = [this](const QString& msg) {
            emit logMessage(msg);
        };
    }

    fs::path base_clone_dir = fs::temp_directory_path() / ("github_clones_" + generateRandomSuffix());
    fs::create_directories(base_clone_dir);

    for (const json& repo : repos) {
        if (!repo["fork"].get<bool>()) {
            emit logMessage("Cloning: " + QString::fromStdString(repo["name"]));
            cloneQueue.push(repo);
        }
    }

    unsigned int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> clonePool, analyzePool;

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

    // Disable callback after finishing logging
    logCallback = nullptr;

    // ===== Final result summary =====
    std::ostringstream summary;
    summary << "===== Final Result =====\n\n";
    summary << "Language Breakdown (by lines of code):\n";

    int total = 0;
    for (const std::pair<const std::string, int>& entry : lang_totals) total += entry.second;

    std::vector<std::pair<std::string, int>> sorted(lang_totals.begin(), lang_totals.end());
    std::sort(sorted.begin(), sorted.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
        return a.second > b.second;
    });

    for (const std::pair<std::string, int>& entry : sorted) {
        const std::string& lang = entry.first;
        int lines = entry.second;
        summary << lang << ": " << (100.0 * lines / total) << "% (" << lines << " lines)\n";
    }

    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    summary << "\nTotal runtime: " << elapsed << " seconds\n";

    std::error_code ec;
    fs::remove_all(base_clone_dir, ec);

    emit finalResult(QString::fromStdString(summary.str()));
    emit finished();
}
