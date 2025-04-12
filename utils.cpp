#include "utils.hpp"
#include <random>

std::string generateRandomSuffix(std::size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    std::string result;
    for (std::size_t i = 0; i < length; ++i)
        result += charset[dist(rng)];

    return result;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
