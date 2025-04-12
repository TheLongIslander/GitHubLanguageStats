#ifndef ANALYZE_HPP
#define ANALYZE_HPP

#include <string>
#include <nlohmann/json.hpp>

// Runs cloc on the specified path and returns parsed JSON result
nlohmann::json runClocAndParse(const std::string& path);

// Multithreaded analyzer worker function
void analyzeWorker();

#endif // ANALYZE_HPP
