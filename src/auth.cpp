#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "auth.hpp"
#include "utils.hpp"


using json = nlohmann::json;

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

std::string exchangeCodeForToken(const std::string& code) {
    const std::string clientId = "Ov23liVQ3EBVLbcP681k";
    const std::string clientSecret = getClientSecret().toStdString();  // Assuming getClientSecret() returns QString

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        std::string postFields =
            "client_id=" + clientId +
            "&client_secret=" + clientSecret +
            "&code=" + code +
            "&redirect_uri=http://localhost:8000/callback";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "User-Agent: CppApp");

        curl_easy_setopt(curl, CURLOPT_URL, "https://github.com/login/oauth/access_token");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "OAuth exchange failed: " << curl_easy_strerror(res) << "\n";

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();

    try {
        json obj = json::parse(readBuffer);
        if (obj.contains("access_token")) {
            return obj["access_token"];
        } else {
            std::cerr << "No access_token in OAuth response.\n";
        }
    } catch (...) {
        std::cerr << "Failed to parse token exchange response.\n";
    }

    return "";
}
