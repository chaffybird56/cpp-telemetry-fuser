#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>

namespace cpp_service {

class Service {
public:
    Service();
    ~Service() = default;
    
    // Core service operations
    std::string health_check() const;
    double fuse_readings(const std::vector<double>& readings) const;
    
    // Configuration
    void set_config(const std::string& config_json);
    std::string get_config() const;
    
    // Statistics
    struct Stats {
        uint64_t total_requests = 0;
        uint64_t successful_requests = 0;
        uint64_t failed_requests = 0;
        double average_fused_value = 0.0;
        std::chrono::steady_clock::time_point start_time;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    // Configuration
    struct Config {
        double outlier_threshold = 3.0;  // Standard deviations
        double min_confidence = 0.8;     // Minimum confidence for fusion
        bool enable_outlier_detection = true;
    };
    
    Config config_;
    mutable std::atomic<uint64_t> total_requests_{0};
    mutable std::atomic<uint64_t> successful_requests_{0};
    mutable std::atomic<uint64_t> failed_requests_{0};
    mutable std::atomic<uint64_t> sum_fused_values_{0};
    mutable std::atomic<uint64_t> fused_count_{0};
    const std::chrono::steady_clock::time_point start_time_;
    
    // Fusion algorithms
    double weighted_average(const std::vector<double>& readings) const;
    double median_filter(const std::vector<double>& readings) const;
    std::vector<double> detect_outliers(const std::vector<double>& readings) const;
    double calculate_confidence(const std::vector<double>& readings, const std::vector<double>& filtered) const;
    
    // Utility functions
    double calculate_mean(const std::vector<double>& values) const;
    double calculate_std_dev(const std::vector<double>& values, double mean) const;
    void sort_vector(std::vector<double>& values) const;
};

} // namespace cpp_service