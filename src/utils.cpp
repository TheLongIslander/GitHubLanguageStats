#include <random>
#include <algorithm>

#include "utils.hpp"

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

QString getClientSecret() {
    QByteArray encoded = "BkQfXCgARmUBAUJKVHhWEGdXUENbZUVQeFcfRVUBMEtVLgRPFlZTNg==";  // example from Python script
    QByteArray combined = QByteArray::fromBase64(encoded);

    int mid = combined.size() / 2;
    QByteArray part1 = combined.left(mid);
    QByteArray part2 = combined.mid(mid);

    std::reverse(part1.begin(), part1.end());  // reverse the first half
    QByteArray full = part1 + part2;

    QByteArray key = "s3cUr3Key";  // Multi-byte XOR key
    for (int i = 0; i < full.size(); ++i)
        full[i] ^= key[i % key.size()];

    return QString::fromUtf8(full);
}
