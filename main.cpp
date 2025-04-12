#include <iostream>
#include <string>
#include <unordered_map>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <git2.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::mutex lang_mutex, clone_mutex, analyze_mutex, cout_mutex;
std::condition_variable analyze_cv;
bool cloning_done = false;

std::unordered_map<std::string, int> lang_totals;
std::queue<json> cloneQueue;
std::queue<std::string> analyzeQueue;

std::string generateRandomSuffix(size_t length = 8) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string result;
    for (size_t i = 0; i < length; ++i) result += charset[dist(rng)];
    return result;
}

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
    while (fgets(buffer, sizeof(buffer), pipe)) output << buffer;
    pclose(pipe);

    std::string out = output.str();
    size_t start = out.find('{'), end = out.rfind('}');
    if (start == std::string::npos || end == std::string::npos) return {};
    try {
        return json::parse(out.substr(start, end - start + 1));
    } catch (...) {
        return {};
    }
}

static int cred_acquire_cb(git_credential** out, const char*, const char*, unsigned int, void* payload) {
    const std::string* token = static_cast<std::string*>(payload);
    return git_credential_userpass_plaintext_new(out, "x-access-token", token->c_str());
}

bool cloneWithLibgit2(const std::string& url, const std::string& path, const std::string& token, bool use_auth) {
    git_repository* repo = nullptr;
    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    if (use_auth) {
        git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
        callbacks.credentials = cred_acquire_cb;
        callbacks.payload = const_cast<std::string*>(&token);
        opts.fetch_opts.callbacks = callbacks;
    }

    int error = git_clone(&repo, url.c_str(), path.c_str(), &opts);
    if (error != 0) {
        const git_error* e = git_error_last();
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "libgit2 error: " << (e && e->message ? e->message : "unknown") << "\n";
        return false;
    }

    git_repository_free(repo);
    return true;
}

void cloneWorker(const fs::path& base_clone_dir, const std::string& token, bool use_auth) {
    while (true) {
        json repo;
        {
            std::lock_guard<std::mutex> lock(clone_mutex);
            if (cloneQueue.empty()) break;
            repo = cloneQueue.front();
            cloneQueue.pop();
        }

        std::string name = repo["name"];
        std::string url = repo["clone_url"];
        std::string repo_path = (base_clone_dir / name).string();

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Cloning: " << name << "\n";
        }

        if (cloneWithLibgit2(url, repo_path, token, use_auth)) {
            std::lock_guard<std::mutex> lock(analyze_mutex);
            analyzeQueue.push(repo_path);
            analyze_cv.notify_one();
        }
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
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Analyzing: " << path << "\n";
        }

        json cloc_data = runClocAndParse(path);
        if (!cloc_data.is_object()) continue;

        std::lock_guard<std::mutex> lock2(lang_mutex);
        for (auto& [lang, stats] : cloc_data.items()) {
            if (lang == "header" || lang == "SUM") continue;
            lang_totals[lang] += stats["code"].get<int>();
        }
    }
}

int main() {
    git_libgit2_init();

    std::string input, token_or_username;
    bool using_token = false;
    json repos;

    // Prompt until valid GitHub credentials or username
    while (true) {
        std::cout << "Enter GitHub token (or press Enter to use public username): ";
        std::getline(std::cin, input);
        if (!input.empty()) {
            using_token = true;
            token_or_username = input;
        } else {
            std::cout << "Enter GitHub username: ";
            std::getline(std::cin, token_or_username);
            using_token = false;
        }

        std::string response = fetchRepos(token_or_username, using_token);
        std::cout << "RAW API RESPONSE:\n" << response << "\n";

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

    fs::path temp_base = fs::temp_directory_path();
    fs::path base_clone_dir = temp_base / ("github_clones_" + generateRandomSuffix());
    fs::create_directories(base_clone_dir);

    for (const auto& repo : repos)
        if (!repo["fork"].get<bool>()) cloneQueue.push(repo);

    unsigned int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> clonePool, analyzePool;

    for (unsigned int i = 0; i < threads; ++i)
        analyzePool.emplace_back(analyzeWorker);
    for (unsigned int i = 0; i < threads; ++i)
        clonePool.emplace_back(cloneWorker, std::ref(base_clone_dir), std::ref(token_or_username), using_token);

    for (auto& t : clonePool) t.join();
    {
        std::lock_guard<std::mutex> lock(analyze_mutex);
        cloning_done = true;
        analyze_cv.notify_all();
    }
    for (auto& t : analyzePool) t.join();

    int total = 0;
    for (const auto& [_, lines] : lang_totals) total += lines;

    std::cout << "\nLanguage Breakdown (by lines of code):\n";
    for (const auto& [lang, lines] : lang_totals)
        printf("%-20s : %6.2f%% (%d lines)\n", lang.c_str(), (100.0 * lines) / total, lines);

    std::error_code ec;
    fs::remove_all(base_clone_dir, ec);
    if (ec) std::cerr << "Failed to delete temp dir: " << ec.message() << "\n";

    git_libgit2_shutdown();
    return 0;
}

