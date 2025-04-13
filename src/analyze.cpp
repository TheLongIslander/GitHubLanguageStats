#include <iostream>
#include <cstdio>
#include <mutex>
#include <queue>
#include <sstream>
#include <filesystem>
#include <condition_variable>

#include "analyze.hpp"
#include "globals.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

json runClocAndParse(const std::string& path) {
    std::string cmd = "cloc --json --quiet "
                      "--exclude-lang=Text,JSON,CSV "
                      "--exclude-dir=node_modules,dist,build,vendor,.git "
                      "--not-match-d=.*\\.d\\.ts$ "
                      "\"" + path + "\" 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};

    std::ostringstream output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output << buffer;
    }
    pclose(pipe);

    std::string out = output.str();
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

        std::lock_guard<std::mutex> langLock(lang_mutex);
        for (nlohmann::json::iterator it = cloc_data.begin(); it != cloc_data.end(); ++it) {
            const std::string& lang = it.key();
            const nlohmann::json& stats = it.value();

            if (lang == "header" || lang == "SUM") continue;

            lang_totals[lang] += stats["code"].get<int>();
        }
    }
}
