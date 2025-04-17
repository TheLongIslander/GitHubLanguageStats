#include <iostream>
#include <cstdio>
#include <mutex>
#include <queue>
#include <sstream>
#include <filesystem>
#include <condition_variable>
#include <unordered_map>

#include "analyze.hpp"
#include "globals.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

// Run cloc with internal parallelism enabled via --processes=N
json runClocAndParse(const std::string& path) {
    std::function<std::string(bool)> runCloc = [&](bool use_parallel) -> std::string {
        std::string cmd = "cloc --json --quiet ";

#ifdef __unix__
        if (use_parallel) {
            unsigned int n_cores = std::thread::hardware_concurrency();
            if (n_cores == 0) n_cores = 2;
            cmd += "--processes=" + std::to_string(n_cores) + " ";
        }
#endif

        cmd += "--exclude-lang=Text,JSON,CSV "
               "--exclude-dir=node_modules,dist,build,vendor,.git "
               "--not-match-d=.*\\.d\\.ts$ "
               "\"" + path + "\"";

#ifndef _WIN32
        cmd += " 2>&1"; // Capture stderr too
#endif

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";

        std::ostringstream output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output << buffer;
        }
        pclose(pipe);

        return output.str();
    };

    // First attempt: with --processes if on UNIX
    std::string out = runCloc(true);

    // Fallback if error occurred
    if (out.find("Parallel::ForkManager") != std::string::npos ||
        out.find("Can't locate Parallel/ForkManager.pm") != std::string::npos) {
        out = runCloc(false);
    }

    size_t start = out.find('{');
    size_t end = out.rfind('}');
    if (start == std::string::npos || end == std::string::npos) return {};

    try {
        return json::parse(out.substr(start, end - start + 1));
    } catch (...) {
        return {};
    }
}

void analyzeWorker() {
    while (true) {
        std::unique_lock<std::mutex> lock(analyze_mutex);
        analyze_cv.wait(lock, [] {
            return !analyzeQueue.empty() || cloning_done;
        });

        if (analyzeQueue.empty()) {
            if (cloning_done) return;
            continue;
        }

        std::string path = analyzeQueue.front();
        analyzeQueue.pop();
        lock.unlock();

        {
            std::lock_guard<std::mutex> coutLock(cout_mutex);
            std::cout << "Analyzing: " << path << "\n";
        }

        json cloc_data = runClocAndParse(path);
        if (!cloc_data.is_object()) continue;

        std::unordered_map<std::string, int> local_totals;

        for (json::iterator it = cloc_data.begin(); it != cloc_data.end(); ++it) {
            const std::string& lang = it.key();
            if (lang == "header" || lang == "SUM") continue;

            const json& stats = it.value();
            if (stats.contains("code")) {
                local_totals[lang] += stats["code"].get<int>();
            }
        }

        {
            std::lock_guard<std::mutex> langLock(lang_mutex);
            for (const std::pair<const std::string, int>& entry : local_totals)
                lang_totals[entry.first] += entry.second;
        }
    }
}
