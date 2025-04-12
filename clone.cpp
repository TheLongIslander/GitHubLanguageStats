#include "clone.hpp"
#include "globals.hpp"

#include <git2.h>
#include <mutex>
#include <iostream>
#include <queue>
#include <condition_variable>

namespace fs = std::filesystem;
using json = nlohmann::json;

int cred_acquire_cb(git_credential** out, const char*, const char*, unsigned int, void* payload) {
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
