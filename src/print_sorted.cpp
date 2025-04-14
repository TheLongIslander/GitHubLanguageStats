#include "print_sorted.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdio>

void printSortedLanguageStats(const std::unordered_map<std::string, int>& lang_totals) {
    std::vector<std::pair<std::string, int>> sorted(lang_totals.begin(), lang_totals.end());

    int total = 0;
    for (const auto& pair : sorted) total += pair.second;

    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    std::cout << "\nLanguage Breakdown (by lines of code):\n";
    for (const auto& pair : sorted) {
        const std::string& lang = pair.first;
        int lines = pair.second;
        printf("%-20s : %6.2f%% (%d lines)\n", lang.c_str(), (100.0 * lines) / total, lines);
    }
}
