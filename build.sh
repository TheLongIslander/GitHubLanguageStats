#!/bin/bash

set -e  # Exit immediately if any command fails

# ANSI color codes
GREEN="\033[0;32m"
BLUE="\033[1;34m"
YELLOW="\033[1;33m"
RESET="\033[0m"

# Correct dependency names for Homebrew
DEPENDENCIES=("libgit2" "curl" "nlohmann-json" "qt@6" "cloc")

echo -e "${BLUE}Checking and installing dependencies if needed...${RESET}"
for dep in "${DEPENDENCIES[@]}"; do
    if ! brew list --versions "$dep" > /dev/null 2>&1; then
        echo -e "${YELLOW}Installing ${dep}...${RESET}"
        brew install "$dep"
    else
        echo -e "${GREEN}${dep} is already installed.${RESET}"
    fi
done

BUILD_DIR="build"

echo -e "${BLUE}Entering build directory...${RESET}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${YELLOW}Cleaning build directory...${RESET}"
rm -rf ./*

echo -e "${BLUE}Running CMake...${RESET}"
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"

echo -e "${BLUE}Building project...${RESET}"
make

echo -e "${GREEN}Build complete.${RESET}"
