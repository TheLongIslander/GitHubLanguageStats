# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Ih -I/usr/local/opt/libgit2/include

# Linker flags
LDFLAGS = -L/usr/local/opt/libgit2/lib -lcurl -lgit2

# Directories
SRC_DIR = src
BIN_DIR = bin

# Sources and objects
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRC))
TARGET = $(BIN_DIR)/github_stats

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

# Compile .cpp to .o
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)

# Run
run: all
	./$(TARGET)
