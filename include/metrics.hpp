#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace cpp_service {

class Metrics {
public:
    Metrics();
    
    // Counter operations
    void increment_counter(const std::string& name, const std::string& labels = "");
    void add_to_counter(const std::string& name, double value, const std::string& labels = "");
    
    // Histogram operations
    void observe_histogram(const std::string& name, double value, const std::string& labels = "");
    
    // Gauge operations
    void set_gauge(const std::string& name, double value, const std::string& labels = "");
    
    // Get metrics in Prometheus format
    std::string get_prometheus_metrics() const;
    
    // Get metrics in JSON format
    std::string get_json_metrics() const;
    
    // Reset all metrics (mainly for testing)
    void reset();
    
private:
    struct Counter {
        std::atomic<uint64_t> value{0};
        std::string labels;
    };
    
    struct Histogram {
        std::atomic<uint64_t> count{0};
        std::atomic<uint64_t> sum{0};
        std::vector<double> buckets;
        std::string labels;
        
        Histogram() : buckets(10, 0.0) {}  // 10 buckets for simplicity
    };
    
    struct Gauge {
        std::atomic<double> value{0.0};
        std::string labels;
    };
    
    mutable std::mutex metrics_mutex_;
    std::map<std::string, Counter> counters_;
    std::map<std::string, Histogram> histograms_;
    std::map<std::string, Gauge> gauges_;
    
    std::string format_labels(const std::string& labels) const;
};

// Global metrics instance
Metrics& get_metrics();

// RAII timer for measuring request duration
class RequestTimer {
public:
    RequestTimer(const std::string& metric_name, const std::string& labels = "");
    ~RequestTimer();
    
private:
    std::string metric_name_;
    std::string labels_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace cpp_service