#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <condition_variable>
#include <nlohmann/json.hpp>

extern std::mutex lang_mutex;
extern std::mutex clone_mutex;
extern std::mutex analyze_mutex;
extern std::mutex cout_mutex;

extern std::condition_variable analyze_cv;
extern bool cloning_done;

extern std::unordered_map<std::string, int> lang_totals;
extern std::queue<nlohmann::json> cloneQueue;
extern std::queue<std::string> analyzeQueue;

#endif // GLOBALS_HPP
