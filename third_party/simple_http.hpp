#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <sstream>
#include <iostream>
#include <memory>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace simple_http {

struct Request {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    std::string get_header(const std::string& name) const {
        auto it = headers.find(name);
        return (it != headers.end()) ? it->second : "";
    }
};

struct Response {
    int status_code = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    Response& set_header(const std::string& name, const std::string& value) {
        headers[name] = value;
        return *this;
    }
    
    Response& json(const std::string& json_str) {
        body = json_str;
        set_header("Content-Type", "application/json");
        return *this;
    }
    
    Response& text(const std::string& text_str) {
        body = text_str;
        set_header("Content-Type", "text/plain");
        return *this;
    }
};

using Handler = std::function<void(const Request&, Response&)>;

class Server {
public:
    Server(int port = 8080) : port_(port), running_(false) {}
    
    void get(const std::string& path, Handler handler) {
        routes_["GET"][path] = handler;
    }
    
    void post(const std::string& path, Handler handler) {
        routes_["POST"][path] = handler;
    }
    
    void run() {
        running_ = true;
        
        // Simple socket server implementation
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            throw std::runtime_error("Failed to create socket");
        }
        
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            throw std::runtime_error("Failed to set socket options");
        }
        
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == -1) {
            throw std::runtime_error("Failed to bind socket");
        }
        
        if (listen(server_fd, 10) == -1) {
            throw std::runtime_error("Failed to listen on socket");
        }
        
        std::cout << "Server listening on port " << port_ << std::endl;
        
        while (running_) {
            sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            int client_fd = accept(server_fd, (sockaddr*)&client_address, &client_len);
            
            if (client_fd == -1) {
                if (running_) {
                    std::cerr << "Failed to accept connection" << std::endl;
                }
                continue;
            }
            
            // Handle request in thread
            std::thread([this, client_fd]() {
                handle_client(client_fd);
                close(client_fd);
            }).detach();
        }
        
        close(server_fd);
    }
    
    void stop() {
        running_ = false;
    }
    
private:
    void handle_client(int client_fd) {
        char buffer[4096] = {0};
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) return;
        
        buffer[bytes_read] = '\0';
        std::string request_str(buffer);
        
        auto request = parse_request(request_str);
        Response response;
        
        auto method_it = routes_.find(request.method);
        if (method_it != routes_.end()) {
            auto path_it = method_it->second.find(request.path);
            if (path_it != method_it->second.end()) {
                try {
                    path_it->second(request, response);
                } catch (const std::exception& e) {
                    response.status_code = 500;
                    response.text("Internal Server Error: " + std::string(e.what()));
                }
            } else {
                response.status_code = 404;
                response.text("Not Found");
            }
        } else {
            response.status_code = 405;
            response.text("Method Not Allowed");
        }
        
        send_response(client_fd, response);
    }
    
    Request parse_request(const std::string& request_str) {
        Request request;
        std::istringstream stream(request_str);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            std::istringstream line_stream(line);
            line_stream >> request.method >> request.path;
        }
        
        // Parse headers
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string name = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                // Trim whitespace
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t\r") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t\r") + 1);
                request.headers[name] = value;
            }
        }
        
        // Parse body
        std::ostringstream body_stream;
        while (std::getline(stream, line)) {
            body_stream << line;
            if (!stream.eof()) body_stream << "\n";
        }
        request.body = body_stream.str();
        
        return request;
    }
    
    void send_response(int client_fd, const Response& response) {
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 " << response.status_code << " OK\r\n";
        
        for (const auto& header : response.headers) {
            response_stream << header.first << ": " << header.second << "\r\n";
        }
        
        response_stream << "Content-Length: " << response.body.length() << "\r\n";
        response_stream << "Connection: close\r\n\r\n";
        response_stream << response.body;
        
        std::string response_str = response_stream.str();
        send(client_fd, response_str.c_str(), response_str.length(), 0);
    }
    
    int port_;
    std::atomic<bool> running_;
    std::unordered_map<std::string, std::unordered_map<std::string, Handler>> routes_;
};

} // namespace simple_http
