#ifndef CLONE_HPP
#define CLONE_HPP

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <git2.h>

void cloneWorker(const std::filesystem::path& base_clone_dir, const std::string& token, bool use_auth);
bool cloneWithLibgit2(const std::string& url, const std::string& path, const std::string& token, bool use_auth);
int cred_acquire_cb(git_credential** out, const char* url, const char* username, unsigned int allowed_types, void* payload);

#endif // CLONE_HPP
