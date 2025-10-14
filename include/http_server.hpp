#pragma once

#include "service.hpp"
#include "metrics.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <map>

namespace cpp_service {

class HttpServer {
public:
    HttpServer(int port, Service* service);
    ~HttpServer() = default;
    
    void run();
    void stop();
    
private:
    int port_;
    Service* service_;
    std::atomic<bool> running_;
    
    std::string parse_json_array(const std::string& json_str, std::vector<double>& readings);
    std::string create_json_response(const std::string& status, const std::string& message = "", 
                                   const std::map<std::string, std::string>& data = {});
};

} // namespace cpp_service
