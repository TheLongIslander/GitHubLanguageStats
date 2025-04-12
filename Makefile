# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I/usr/local/opt/libgit2/include

# Linker flags
LDFLAGS = -L/usr/local/opt/libgit2/lib -lcurl -lgit2

# Source files
SRC = main.cpp auth.cpp clone.cpp analyze.cpp utils.cpp globals.cpp
OBJ = $(SRC:.cpp=.o)

# Output executable
TARGET = github_stats

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

# Compile .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: all
	./$(TARGET)
