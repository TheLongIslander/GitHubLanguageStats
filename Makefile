# Get Homebrew prefix (works on Intel and Apple Silicon)
BREW_PREFIX := $(shell brew --prefix)

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -flto -Ih -I$(BREW_PREFIX)/include
LDFLAGS = -L$(BREW_PREFIX)/lib -flto

# Directories
SRC_DIR = src
BIN_DIR = bin

# Sources and objects
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRC))
TARGET = $(BIN_DIR)/github_stats

# Try to use pkg-config for libgit2 and curl
PKG_CONFIG := $(shell which pkg-config 2>/dev/null)

ifeq ($(PKG_CONFIG),)
$(warning pkg-config not found! You may need to install dependencies manually.)
else
CXXFLAGS += $(shell pkg-config --cflags libgit2 libcurl)
LDFLAGS  += $(shell pkg-config --libs libgit2 libcurl)
endif

# Default target
all: deps $(TARGET)

# Build target
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

# Compile .cpp to .o
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Install dependencies
deps:
	@echo "Checking and installing dependencies..."
	@if [ "$$(uname)" = "Darwin" ]; then \
		echo "Detected macOS"; \
		if ! command -v brew >/dev/null 2>&1; then \
			echo "Homebrew not found. Please install it from https://brew.sh"; \
			exit 1; \
		fi; \
		if ! command -v pkg-config >/dev/null 2>&1; then echo "Installing pkg-config..."; brew install pkg-config; fi; \
		if ! brew list libgit2 >/dev/null 2>&1; then echo "Installing libgit2..."; brew install libgit2; fi; \
		if ! brew list curl >/dev/null 2>&1; then echo "Installing curl..."; brew install curl; fi; \
		if ! brew list nlohmann-json >/dev/null 2>&1; then echo "Installing nlohmann-json..."; brew install nlohmann-json; fi; \
		if ! brew list cloc >/dev/null 2>&1; then echo "Installing cloc..."; brew install cloc; fi; \
	elif [ "$$(uname)" = "Linux" ]; then \
		echo "Detected Linux"; \
		if ! command -v pkg-config >/dev/null 2>&1; then sudo apt update && sudo apt install -y pkg-config; fi; \
		dpkg -s libgit2-dev >/dev/null 2>&1 || sudo apt install -y libgit2-dev; \
		dpkg -s libcurl4-openssl-dev >/dev/null 2>&1 || sudo apt install -y libcurl4-openssl-dev; \
		dpkg -s nlohmann-json-dev >/dev/null 2>&1 || sudo apt install -y nlohmann-json-dev; \
		dpkg -s cloc >/dev/null 2>&1 || sudo apt install -y cloc; \
	else \
		echo "Unsupported OS. Please install dependencies manually."; \
	fi

# Clean
clean:
	rm -rf $(BIN_DIR)

# Run
run: all
	./$(TARGET)
