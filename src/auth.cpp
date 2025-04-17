#include <iostream>
#include <curl/curl.h>
#include "auth.hpp"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string fetchRepos(const std::string& identifier, bool using_token) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = using_token
        ? "https://api.github.com/user/repos?per_page=100&type=all"
        : "https://api.github.com/users/" + identifier + "/repos?per_page=100";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = nullptr;
        if (using_token)
            headers = curl_slist_append(headers, ("Authorization: token " + identifier).c_str());
        headers = curl_slist_append(headers, "User-Agent: CppApp");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << "\n";

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
    return readBuffer;
}

json promptAndFetchRepos(std::string& identifier, bool& using_token) {
    json repos;

    // ✅ Skip prompt if identifier is already provided (e.g., from GUI)
    if (!identifier.empty()) {
        using_token = identifier.length() > 30; // crude check
        std::string response = fetchRepos(identifier, using_token);

        try {
            repos = json::parse(response);
            if (!repos.is_array()) {
                std::cerr << "GitHub API error: " << (repos.contains("message") ? repos["message"] : "Unexpected format") << "\n";
                return {};
            }
        } catch (...) {
            std::cerr << "Failed to parse GitHub API response.\n";
            return {};
        }

        return repos;
    }

    // 👇 If GUI didn't set it, fallback to terminal prompt (for --nogui mode)
    while (true) {
        std::string input;
        std::cout << "Enter GitHub token (or press Enter to use public username): ";
        std::getline(std::cin, input);

        if (!input.empty()) {
            using_token = true;
            identifier = input;
        } else {
            std::cout << "Enter GitHub username: ";
            std::getline(std::cin, identifier);
            using_token = false;
        }

        std::string response = fetchRepos(identifier, using_token);
        try {
            repos = json::parse(response);
            if (repos.is_array()) break;
            if (repos.contains("message")) {
                std::cerr << "GitHub API Error: " << repos["message"] << "\n";
            } else {
                std::cerr << "Unexpected response format.\n";
            }
        } catch (...) {
            std::cerr << "Failed to parse JSON.\n";
        }

        std::cerr << "Please try again.\n\n";
    }

    return repos;
}
