#include "service.hpp"
#include "metrics.hpp"
#include "http_server.hpp"
#include <iostream>
#include <fstream>
#include <signal.h>
#include <memory>

namespace {
    std::unique_ptr<cpp_service::HttpServer> server;
    std::unique_ptr<cpp_service::Service> service;
    
    void signal_handler(int signal) {
        std::cout << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
        if (server) {
            server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    std::string config_file;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --port PORT      Port to listen on (default: 8080)\n";
            std::cout << "  --config FILE    Configuration file (JSON)\n";
            std::cout << "  --help           Show this help message\n";
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Initialize service
        service = std::make_unique<cpp_service::Service>();
        
        // Load configuration if provided
        if (!config_file.empty()) {
            std::ifstream config_stream(config_file);
            if (config_stream.is_open()) {
                std::string config_content((std::istreambuf_iterator<char>(config_stream)),
                                         std::istreambuf_iterator<char>());
                service->set_config(config_content);
            } else {
                std::cerr << "Warning: Could not open config file: " << config_file << std::endl;
            }
        }
        
        // Initialize HTTP server
        server = std::make_unique<cpp_service::HttpServer>(port, service.get());
        
        std::cout << "Starting C++ service on port " << port << std::endl;
        server->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start service: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Service stopped" << std::endl;
    return 0;
}