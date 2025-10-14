#include "service.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace cpp_service {

Service::Service() : start_time_(std::chrono::steady_clock::now()) {
    // Service initialized
}

std::string Service::health_check() const {
    return "ok";
}

double Service::fuse_readings(const std::vector<double>& readings) const {
    if (readings.empty()) {
        return 0.0;
    }
    
    total_requests_.fetch_add(1, std::memory_order_relaxed);
    
    try {
        std::vector<double> processed_readings = readings;
        
        // Apply outlier detection if enabled
        if (config_.enable_outlier_detection && readings.size() > 2) {
            auto outliers = detect_outliers(readings);
            if (!outliers.empty()) {
                // Remove outliers
                processed_readings.erase(
                    std::remove_if(processed_readings.begin(), processed_readings.end(),
                        [&outliers](double value) {
                            return std::find(outliers.begin(), outliers.end(), value) != outliers.end();
                        }),
                    processed_readings.end()
                );
            }
        }
        
        if (processed_readings.empty()) {
            processed_readings = readings;
        }
        
        // Calculate confidence
        double confidence = calculate_confidence(readings, processed_readings);
        
        // Apply fusion algorithm (weighted average with median filter backup)
        double fused_value;
        if (processed_readings.size() >= 3) {
            // Use median filter for robustness
            fused_value = median_filter(processed_readings);
        } else {
            // Use weighted average for small datasets
            fused_value = weighted_average(processed_readings);
        }
        
        // Update statistics
        sum_fused_values_.fetch_add(static_cast<uint64_t>(fused_value * 1000), std::memory_order_relaxed);
        fused_count_.fetch_add(1, std::memory_order_relaxed);
        successful_requests_.fetch_add(1, std::memory_order_relaxed);
        
        return fused_value;
        
    } catch (const std::exception& e) {
        failed_requests_.fetch_add(1, std::memory_order_relaxed);
        throw;
    }
}

void Service::set_config(const std::string& config_json) {
    // Simple JSON parsing - in production you'd use a proper JSON library
    // For demo purposes, we'll just set some defaults
    // This could be enhanced with actual JSON parsing
}

std::string Service::get_config() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"outlier_threshold\": " << config_.outlier_threshold << ",\n";
    oss << "  \"min_confidence\": " << config_.min_confidence << ",\n";
    oss << "  \"enable_outlier_detection\": " << (config_.enable_outlier_detection ? "true" : "false") << "\n";
    oss << "}";
    return oss.str();
}

Service::Stats Service::get_stats() const {
    Stats stats;
    stats.total_requests = total_requests_.load();
    stats.successful_requests = successful_requests_.load();
    stats.failed_requests = failed_requests_.load();
    
    auto count = fused_count_.load();
    if (count > 0) {
        stats.average_fused_value = static_cast<double>(sum_fused_values_.load()) / (count * 1000);
    }
    
    stats.start_time = start_time_;
    return stats;
}

void Service::reset_stats() {
    total_requests_.store(0);
    successful_requests_.store(0);
    failed_requests_.store(0);
    sum_fused_values_.store(0);
    fused_count_.store(0);
}

double Service::weighted_average(const std::vector<double>& readings) const {
    if (readings.empty()) return 0.0;
    if (readings.size() == 1) return readings[0];
    
    // Calculate weights based on inverse variance
    std::vector<double> weights;
    double mean = calculate_mean(readings);
    double variance = 0.0;
    
    for (double reading : readings) {
        variance += (reading - mean) * (reading - mean);
    }
    variance /= readings.size();
    
    // Use inverse variance as weight (higher variance = lower weight)
    for (double reading : readings) {
        double diff = reading - mean;
        double weight = 1.0 / (1.0 + diff * diff);  // Avoid division by zero
        weights.push_back(weight);
    }
    
    // Normalize weights
    double total_weight = std::accumulate(weights.begin(), weights.end(), 0.0);
    if (total_weight == 0.0) {
        return mean;  // Fallback to simple average
    }
    
    double weighted_sum = 0.0;
    for (size_t i = 0; i < readings.size(); ++i) {
        weighted_sum += readings[i] * weights[i];
    }
    
    return weighted_sum / total_weight;
}

double Service::median_filter(const std::vector<double>& readings) const {
    if (readings.empty()) return 0.0;
    
    std::vector<double> sorted_readings = readings;
    sort_vector(sorted_readings);
    
    size_t n = sorted_readings.size();
    if (n % 2 == 0) {
        return (sorted_readings[n/2 - 1] + sorted_readings[n/2]) / 2.0;
    } else {
        return sorted_readings[n/2];
    }
}

std::vector<double> Service::detect_outliers(const std::vector<double>& readings) const {
    if (readings.size() <= 2) return {};
    
    double mean = calculate_mean(readings);
    double std_dev = calculate_std_dev(readings, mean);
    
    if (std_dev == 0.0) return {};  // All values are the same
    
    std::vector<double> outliers;
    for (double reading : readings) {
        double z_score = std::abs((reading - mean) / std_dev);
        if (z_score > config_.outlier_threshold) {
            outliers.push_back(reading);
        }
    }
    
    return outliers;
}

double Service::calculate_confidence(const std::vector<double>& readings, const std::vector<double>& filtered) const {
    if (readings.empty()) return 0.0;
    
    // Confidence based on how many readings were kept vs filtered
    double retention_rate = static_cast<double>(filtered.size()) / readings.size();
    
    // Additional confidence based on consistency of filtered readings
    if (filtered.size() > 1) {
        double mean = calculate_mean(filtered);
        double std_dev = calculate_std_dev(filtered, mean);
        double coefficient_of_variation = std_dev / std::abs(mean);
        
        // Lower CV = higher confidence
        double consistency_factor = 1.0 / (1.0 + coefficient_of_variation);
        return retention_rate * consistency_factor;
    }
    
    return retention_rate;
}

double Service::calculate_mean(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double Service::calculate_std_dev(const std::vector<double>& values, double mean) const {
    if (values.size() <= 1) return 0.0;
    
    double variance = 0.0;
    for (double value : values) {
        variance += (value - mean) * (value - mean);
    }
    variance /= values.size();
    
    return std::sqrt(variance);
}

void Service::sort_vector(std::vector<double>& values) const {
    std::sort(values.begin(), values.end());
}

} // namespace cpp_service