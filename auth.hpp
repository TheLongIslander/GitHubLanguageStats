#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Prompts user for GitHub token or username and fetches repositories.
json promptAndFetchRepos(std::string& identifier, bool& using_token);

// Performs the actual GitHub API request to fetch repos.
std::string fetchRepos(const std::string& identifier, bool using_token);
