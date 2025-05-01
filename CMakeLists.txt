cmake_minimum_required(VERSION 3.16)
project(GitHubLangStats)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -O3 -march=native -flto")

# ========================
# Dependencies via pkg-config
# ========================
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)
pkg_check_modules(LIBCURL REQUIRED libcurl)
find_package(nlohmann_json REQUIRED)

# ========================
# Qt6 GUI support
# ========================
find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebEngineWidgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ========================
# Include directories
# ========================
include_directories(
    ${LIBGIT2_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/h
)

link_directories(
    ${LIBGIT2_LIBRARY_DIRS}
    ${LIBCURL_LIBRARY_DIRS}
)

# ========================
# Source files
# ========================
set(SOURCES
    src/main.cpp
    src/runTerminalMode.cpp
    src/mainwindow.cpp
    src/worker.cpp
    src/analyze.cpp
    src/auth.cpp
    src/clone.cpp
    src/globals.cpp
    src/utils.cpp
    src/print_sorted.cpp
    src/oauthserver.cpp
    src/pureOAuthServer.cpp
)

set(HEADERS
    h/runTerminalMode.hpp
    h/mainwindow.hpp
    h/worker.hpp
    h/analyze.hpp
    h/auth.hpp
    h/clone.hpp
    h/globals.hpp
    h/utils.hpp
    h/print_sorted.hpp
    h/oauthserver.hpp
    h/pureOAuthServer.hpp
)

# ========================
# Executable target with proper Qt MOC support
# ========================
add_executable(GitHubLangStats ${SOURCES} ${HEADERS})

# ========================
# Link libraries
# ========================
target_link_libraries(GitHubLangStats
    ${LIBGIT2_LIBRARIES}
    ${LIBCURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineWidgets
)


# ========================
# Runtime check for cloc
# ========================
find_program(CLOC_EXECUTABLE cloc REQUIRED)
if(NOT CLOC_EXECUTABLE)
    message(FATAL_ERROR "cloc not found. Please install it (e.g., brew install cloc)")
endif()
