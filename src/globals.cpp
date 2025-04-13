#include "globals.hpp"

std::mutex lang_mutex;
std::mutex clone_mutex;
std::mutex analyze_mutex;
std::mutex cout_mutex;

std::condition_variable analyze_cv;
bool cloning_done = false;

std::unordered_map<std::string, int> lang_totals;
std::queue<nlohmann::json> cloneQueue;
std::queue<std::string> analyzeQueue;
