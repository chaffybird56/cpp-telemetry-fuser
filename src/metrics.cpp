#include "metrics.hpp"
#include <sstream>
#include <iomanip>

namespace cpp_service {

Metrics::Metrics() = default;

void Metrics::increment_counter(const std::string& name, const std::string& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto key = name + "|" + labels;  // Use separator to avoid key collision
    auto& counter = counters_[key];
    if (counter.labels.empty()) {
        counter.labels = labels;
    }
    counter.value.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::add_to_counter(const std::string& name, double value, const std::string& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto key = name + "|" + labels;  // Use separator to avoid key collision
    auto& counter = counters_[key];
    if (counter.labels.empty()) {
        counter.labels = labels;
    }
    counter.value.fetch_add(static_cast<uint64_t>(value), std::memory_order_relaxed);
}

void Metrics::observe_histogram(const std::string& name, double value, const std::string& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto key = name + "|" + labels;  // Use separator to avoid key collision
    
    auto& hist = histograms_[key];
    if (hist.labels.empty()) {
        hist.labels = labels;
    }
    
    hist.count.fetch_add(1, std::memory_order_relaxed);
    hist.sum.fetch_add(static_cast<uint64_t>(value), std::memory_order_relaxed);
    
    // Simple histogram buckets
    size_t bucket_index = 0;
    if (value <= 1.0) bucket_index = 0;
    else if (value <= 5.0) bucket_index = 1;
    else if (value <= 10.0) bucket_index = 2;
    else if (value <= 25.0) bucket_index = 3;
    else if (value <= 50.0) bucket_index = 4;
    else if (value <= 100.0) bucket_index = 5;
    else if (value <= 250.0) bucket_index = 6;
    else if (value <= 500.0) bucket_index = 7;
    else if (value <= 1000.0) bucket_index = 8;
    else bucket_index = 9;
    
    if (bucket_index < hist.buckets.size()) {
        hist.buckets[bucket_index]++;
    }
}

void Metrics::set_gauge(const std::string& name, double value, const std::string& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto key = name + "|" + labels;  // Use separator to avoid key collision
    auto& gauge = gauges_[key];
    if (gauge.labels.empty()) {
        gauge.labels = labels;
    }
    gauge.value.store(value, std::memory_order_relaxed);
}

std::string Metrics::get_prometheus_metrics() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Counters
    for (const auto& [key, counter] : counters_) {
        // Extract metric name from key (before the separator)
        auto separator_pos = key.find("|");
        std::string metric_name = (separator_pos != std::string::npos) ? key.substr(0, separator_pos) : key;
        
        oss << "# HELP " << metric_name << " Total count\n";
        oss << "# TYPE " << metric_name << " counter\n";
        oss << metric_name << format_labels(counter.labels) << " " << counter.value.load() << "\n";
    }
    
    // Histograms
    for (const auto& [key, hist] : histograms_) {
        // Extract metric name from key (before the separator)
        auto separator_pos = key.find("|");
        std::string metric_name = (separator_pos != std::string::npos) ? key.substr(0, separator_pos) : key;
        
        oss << "# HELP " << metric_name << " Request duration histogram\n";
        oss << "# TYPE " << metric_name << " histogram\n";
        
        auto count = hist.count.load();
        auto sum = hist.sum.load();
        
        // Bucket boundaries: 1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf
        std::vector<double> bucket_bounds = {1.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0, 1000.0};
        
        uint64_t cumulative = 0;
        for (size_t i = 0; i < bucket_bounds.size(); ++i) {
            cumulative += hist.buckets[i];
            std::string bucket_labels = hist.labels + ",le=\"" + std::to_string(bucket_bounds[i]) + "\"";
            oss << metric_name << "_bucket" << format_labels(bucket_labels) << " " << cumulative << "\n";
        }
        
        std::string inf_labels = hist.labels + ",le=\"+Inf\"";
        oss << metric_name << "_bucket" << format_labels(inf_labels) << " " << count << "\n";
        oss << metric_name << "_count" << format_labels(hist.labels) << " " << count << "\n";
        oss << metric_name << "_sum" << format_labels(hist.labels) << " " << sum << "\n";
    }
    
    // Gauges
    for (const auto& [key, gauge] : gauges_) {
        // Extract metric name from key (before the separator)
        auto separator_pos = key.find("|");
        std::string metric_name = (separator_pos != std::string::npos) ? key.substr(0, separator_pos) : key;
        
        oss << "# HELP " << metric_name << " Current value\n";
        oss << "# TYPE " << metric_name << " gauge\n";
        oss << metric_name << format_labels(gauge.labels) << " " << gauge.value.load() << "\n";
    }
    
    return oss.str();
}

std::string Metrics::get_json_metrics() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"counters\": {\n";
    
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Counters
    bool first = true;
    for (const auto& [key, counter] : counters_) {
        if (!first) oss << ",\n";
        // Extract metric name from key (before the separator)
        auto separator_pos = key.find("|");
        std::string metric_name = (separator_pos != std::string::npos) ? key.substr(0, separator_pos) : key;
        oss << "    \"" << metric_name << "\": " << counter.value.load();
        first = false;
    }
    
    oss << "\n  },\n";
    oss << "  \"histograms\": {\n";
    
    // Histograms
    first = true;
    for (const auto& [key, hist] : histograms_) {
        if (!first) oss << ",\n";
        // Extract metric name from key (before the separator)
        auto separator_pos = key.find("|");
        std::string metric_name = (separator_pos != std::string::npos) ? key.substr(0, separator_pos) : key;
        oss << "    \"" << metric_name << "\": {\n";
        oss << "      \"count\": " << hist.count.load() << ",\n";
        oss << "      \"sum\": " << hist.sum.load() << "\n";
        oss << "    }";
        first = false;
    }
    
    oss << "\n  },\n";
    oss << "  \"gauges\": {\n";
    
    // Gauges
    first = true;
    for (const auto& [key, gauge] : gauges_) {
        if (!first) oss << ",\n";
        // Extract metric name from key (before the separator)
        auto separator_pos = key.find("|");
        std::string metric_name = (separator_pos != std::string::npos) ? key.substr(0, separator_pos) : key;
        oss << "    \"" << metric_name << "\": " << gauge.value.load();
        first = false;
    }
    
    oss << "\n  }\n";
    oss << "}";
    
    return oss.str();
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    counters_.clear();
    histograms_.clear();
    gauges_.clear();
}

std::string Metrics::format_labels(const std::string& labels) const {
    if (labels.empty()) return "";
    return "{" + labels + "}";
}

// Global metrics instance
static Metrics global_metrics;

Metrics& get_metrics() {
    return global_metrics;
}

// RequestTimer implementation
RequestTimer::RequestTimer(const std::string& metric_name, const std::string& labels)
    : metric_name_(metric_name), labels_(labels), start_time_(std::chrono::steady_clock::now()) {}

RequestTimer::~RequestTimer() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    double duration_ms = duration.count() / 1000.0;
    
    get_metrics().observe_histogram(metric_name_, duration_ms, labels_);
}

} // namespace cpp_service