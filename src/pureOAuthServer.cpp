#include <iostream>
#include <string>
#include <thread>
#include <regex>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>

std::string waitForOAuthCode() {
    const int port = 8000;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};
    std::string code;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        return "";
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return "";
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        return "";
    }

    std::cout << "Waiting for GitHub to redirect to http://localhost:8000/callback...\n";

    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("accept");
        return "";
    }

    read(new_socket, buffer, sizeof(buffer));
    std::string request(buffer);

    std::regex code_regex("GET /callback\\?code=([^ ]+)");
    std::smatch match;
    if (std::regex_search(request, match, code_regex)) {
        code = match[1];
    }

    // Serve static HTML
    std::ifstream file("login_frontend/success.html");
    std::stringstream html;
    if (file.is_open()) {
        html << file.rdbuf();
        file.close();
    } else {
        html << "<html><body><h2>Login successful!</h2><p>But we couldn't load the custom page.</p></body></html>";
    }

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n" +
        html.str();

    send(new_socket, response.c_str(), response.size(), 0);
    close(new_socket);
    close(server_fd);

    return code;
}
