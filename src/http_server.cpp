#include "http_server.hpp"
#include "../third_party/simple_http.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

namespace cpp_service {

HttpServer::HttpServer(int port, Service* service) : port_(port), service_(service), running_(false) {
}

void HttpServer::run() {
    running_ = true;
    
    std::cout << "HTTP Server starting on port " << port_ << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /health" << std::endl;
    std::cout << "  POST /fuse" << std::endl;
    std::cout << "  GET  /metrics" << std::endl;
    std::cout << "  GET  /stats" << std::endl;
    std::cout << "  GET  /config" << std::endl;
    std::cout << "  POST /config" << std::endl;
    std::cout << std::endl;
    std::cout << "Example requests:" << std::endl;
    std::cout << "  curl http://localhost:" << port_ << "/health" << std::endl;
    std::cout << "  curl -X POST http://localhost:" << port_ << "/fuse -H 'Content-Type: application/json' -d '{\"readings\":[12.1,11.9,12.0,12.2]}'" << std::endl;
    std::cout << "  curl http://localhost:" << port_ << "/metrics" << std::endl;
    std::cout << std::endl;
    
    try {
        simple_http::Server server(port_);
        
        // Set up routes
        server.get("/health", [this](const simple_http::Request& req, simple_http::Response& res) {
            RequestTimer timer("request_duration_ms", "endpoint=\"/health\"");
            get_metrics().increment_counter("requests_total", "endpoint=\"/health\"");
            
            std::map<std::string, std::string> data;
            data["status"] = service_->health_check();
            data["version"] = "0.1.0";
            
            res.json(create_json_response("success", "", data));
        });
        
        server.post("/fuse", [this](const simple_http::Request& req, simple_http::Response& res) {
            RequestTimer timer("request_duration_ms", "endpoint=\"/fuse\"");
            get_metrics().increment_counter("requests_total", "endpoint=\"/fuse\"");
            
            try {
                std::vector<double> readings;
                std::string error = parse_json_array(req.body, readings);
                
                if (!error.empty()) {
                    res.status_code = 400;
                    res.json(create_json_response("error", error));
                    get_metrics().increment_counter("errors_total", "endpoint=\"/fuse\",error=\"bad_request\"");
                    return;
                }
                
                if (readings.empty()) {
                    res.status_code = 400;
                    res.json(create_json_response("error", "readings array cannot be empty"));
                    get_metrics().increment_counter("errors_total", "endpoint=\"/fuse\",error=\"empty_readings\"");
                    return;
                }
                
                double fused_value = service_->fuse_readings(readings);
                
                std::map<std::string, std::string> data;
                data["fused_value"] = std::to_string(fused_value);
                data["input_count"] = std::to_string(readings.size());
                data["timestamp"] = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
                
                res.json(create_json_response("success", "", data));
                
            } catch (const std::exception& e) {
                std::cerr << "Error processing fusion request: " << e.what() << std::endl;
                res.status_code = 500;
                res.json(create_json_response("error", "Internal server error"));
                get_metrics().increment_counter("errors_total", "endpoint=\"/fuse\",error=\"internal_error\"");
            }
        });
        
        server.get("/metrics", [this](const simple_http::Request& req, simple_http::Response& res) {
            RequestTimer timer("request_duration_ms", "endpoint=\"/metrics\"");
            get_metrics().increment_counter("requests_total", "endpoint=\"/metrics\"");
            
            res.set_header("Content-Type", "text/plain; version=0.0.4; charset=utf-8");
            res.text(get_metrics().get_prometheus_metrics());
        });
        
        server.get("/stats", [this](const simple_http::Request& req, simple_http::Response& res) {
            RequestTimer timer("request_duration_ms", "endpoint=\"/stats\"");
            get_metrics().increment_counter("requests_total", "endpoint=\"/stats\"");
            
            std::map<std::string, std::string> data;
            data["metrics"] = get_metrics().get_json_metrics();
            
            // Add service statistics
            auto stats = service_->get_stats();
            data["total_requests"] = std::to_string(stats.total_requests);
            data["successful_requests"] = std::to_string(stats.successful_requests);
            data["failed_requests"] = std::to_string(stats.failed_requests);
            data["average_fused_value"] = std::to_string(stats.average_fused_value);
            
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - stats.start_time).count();
            data["uptime_seconds"] = std::to_string(uptime);
            
            res.json(create_json_response("success", "", data));
        });
        
        server.get("/config", [this](const simple_http::Request& req, simple_http::Response& res) {
            RequestTimer timer("request_duration_ms", "endpoint=\"/config\"");
            get_metrics().increment_counter("requests_total", "endpoint=\"/config\"");
            
            res.json(service_->get_config());
        });
        
        server.post("/config", [this](const simple_http::Request& req, simple_http::Response& res) {
            RequestTimer timer("request_duration_ms", "endpoint=\"/config\"");
            get_metrics().increment_counter("requests_total", "endpoint=\"/config\"");
            
            try {
                service_->set_config(req.body);
                res.json(create_json_response("success", "Configuration updated"));
            } catch (const std::exception& e) {
                res.status_code = 400;
                res.json(create_json_response("error", e.what()));
                get_metrics().increment_counter("errors_total", "endpoint=\"/config\",error=\"invalid_config\"");
            }
        });
        
        std::cout << "HTTP Server running on port " << port_ << std::endl;
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start HTTP server: " << e.what() << std::endl;
        running_ = false;
    }
}

void HttpServer::stop() {
    running_ = false;
}

std::string HttpServer::parse_json_array(const std::string& json_str, std::vector<double>& readings) {
    // Simple JSON array parsing for demo purposes
    // In production, use a proper JSON library
    
    readings.clear();
    
    // Find the "readings" array
    size_t readings_pos = json_str.find("\"readings\"");
    if (readings_pos == std::string::npos) {
        return "Missing 'readings' field";
    }
    
    size_t array_start = json_str.find('[', readings_pos);
    if (array_start == std::string::npos) {
        return "Invalid JSON array format";
    }
    
    size_t array_end = json_str.find(']', array_start);
    if (array_end == std::string::npos) {
        return "Unclosed JSON array";
    }
    
    std::string array_content = json_str.substr(array_start + 1, array_end - array_start - 1);
    
    // Parse numbers from the array
    std::istringstream array_stream(array_content);
    std::string number_str;
    
    while (std::getline(array_stream, number_str, ',')) {
        // Trim whitespace
        number_str.erase(0, number_str.find_first_not_of(" \t"));
        number_str.erase(number_str.find_last_not_of(" \t") + 1);
        
        if (!number_str.empty()) {
            try {
                double value = std::stod(number_str);
                readings.push_back(value);
            } catch (const std::exception&) {
                return "Invalid number in readings array: " + number_str;
            }
        }
    }
    
    return "";
}

std::string HttpServer::create_json_response(const std::string& status, const std::string& message, 
                                           const std::map<std::string, std::string>& data) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"status\": \"" << status << "\"";
    
    if (!message.empty()) {
        oss << ",\n  \"message\": \"" << message << "\"";
    }
    
    if (!data.empty()) {
        oss << ",\n  \"data\": {\n";
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) oss << ",\n";
            oss << "    \"" << key << "\": \"" << value << "\"";
            first = false;
        }
        oss << "\n  }";
    }
    
    oss << "\n}";
    return oss.str();
}

} // namespace cpp_service
