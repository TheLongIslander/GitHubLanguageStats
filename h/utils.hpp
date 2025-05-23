#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstddef>
#include <QString>

// Generates a random alphanumeric suffix (default 8 characters)
std::string generateRandomSuffix(std::size_t length = 8);

// CURL write callback for storing API response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

// Obfuscated GitHub client secret
QString getClientSecret();

#endif // UTILS_HPP
