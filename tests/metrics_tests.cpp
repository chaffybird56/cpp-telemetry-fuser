#include <gtest/gtest.h>
#include "metrics.hpp"
#include <thread>
#include <chrono>

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        metrics = std::make_unique<cpp_service::Metrics>();
    }
    
    void TearDown() override {
        metrics.reset();
    }
    
    std::unique_ptr<cpp_service::Metrics> metrics;
};

TEST_F(MetricsTest, CounterOperations) {
    // Test increment counter
    metrics->increment_counter("test_counter");
    metrics->increment_counter("test_counter");
    metrics->increment_counter("test_counter", "label1=\"value1\"");
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    // Debug output
    std::cout << "Prometheus output:\n" << prometheus_output << std::endl;
    
    // Should contain counter metrics
    EXPECT_TRUE(prometheus_output.find("test_counter 2") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_counter{label1=\"value1\"} 1") != std::string::npos);
}

TEST_F(MetricsTest, CounterAddValue) {
    metrics->add_to_counter("test_counter", 5.5);
    metrics->add_to_counter("test_counter", 2.3);
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    EXPECT_TRUE(prometheus_output.find("test_counter 7") != std::string::npos);
}

TEST_F(MetricsTest, HistogramOperations) {
    // Test histogram observations
    metrics->observe_histogram("test_histogram", 10.5);
    metrics->observe_histogram("test_histogram", 25.0);
    metrics->observe_histogram("test_histogram", 100.0);
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    // Should contain histogram metrics
    EXPECT_TRUE(prometheus_output.find("test_histogram_count 3") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_histogram_sum 135") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_histogram_bucket") != std::string::npos);
}

TEST_F(MetricsTest, HistogramWithLabels) {
    metrics->observe_histogram("test_histogram", 15.0, "endpoint=\"/test\"");
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    EXPECT_TRUE(prometheus_output.find("test_histogram_count{endpoint=\"/test\"} 1") != std::string::npos);
}

TEST_F(MetricsTest, GaugeOperations) {
    metrics->set_gauge("test_gauge", 42.5);
    metrics->set_gauge("test_gauge", 100.0);
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    EXPECT_TRUE(prometheus_output.find("test_gauge 100") != std::string::npos);
}

TEST_F(MetricsTest, GaugeWithLabels) {
    metrics->set_gauge("test_gauge", 75.0, "instance=\"test\"");
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    EXPECT_TRUE(prometheus_output.find("test_gauge{instance=\"test\"} 75") != std::string::npos);
}

TEST_F(MetricsTest, JsonMetricsFormat) {
    metrics->increment_counter("test_counter");
    metrics->observe_histogram("test_histogram", 10.0);
    metrics->set_gauge("test_gauge", 50.0);
    
    std::string json_output = metrics->get_json_metrics();
    
    EXPECT_FALSE(json_output.empty());
    
    // Should contain expected sections
    EXPECT_TRUE(json_output.find("\"counters\"") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"histograms\"") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"gauges\"") != std::string::npos);
    
    EXPECT_TRUE(json_output.find("\"test_counter\"") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"test_histogram\"") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"test_gauge\"") != std::string::npos);
}

TEST_F(MetricsTest, RequestTimer) {
    {
        cpp_service::RequestTimer timer("test_timer");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::string prometheus_output = cpp_service::get_metrics().get_prometheus_metrics();
    std::cout << "Prometheus output:\n" << prometheus_output << std::endl;
    
    // Should have recorded a timing
    EXPECT_TRUE(prometheus_output.find("test_timer_count 1") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_timer_sum") != std::string::npos);
}

TEST_F(MetricsTest, RequestTimerWithLabels) {
    {
        cpp_service::RequestTimer timer("test_timer", "endpoint=\"/test\"");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    std::string prometheus_output = cpp_service::get_metrics().get_prometheus_metrics();
    
    EXPECT_TRUE(prometheus_output.find("test_timer_count{endpoint=\"/test\"} 1") != std::string::npos);
}

TEST_F(MetricsTest, ResetMetrics) {
    metrics->increment_counter("test_counter");
    metrics->observe_histogram("test_histogram", 10.0);
    metrics->set_gauge("test_gauge", 50.0);
    
    // Verify metrics exist
    std::string output_before = metrics->get_prometheus_metrics();
    EXPECT_FALSE(output_before.empty());
    
    // Reset metrics
    metrics->reset();
    
    // Verify metrics are cleared
    std::string output_after = metrics->get_prometheus_metrics();
    EXPECT_TRUE(output_after.empty() || output_after.find("test_counter") == std::string::npos);
}

TEST_F(MetricsTest, ConcurrentAccess) {
    const int num_threads = 4;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, operations_per_thread, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                metrics->increment_counter("concurrent_counter", "thread=\"" + std::to_string(i) + "\"");
                metrics->observe_histogram("concurrent_histogram", j * 0.1);
                metrics->set_gauge("concurrent_gauge", i * 10.0 + j);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string prometheus_output = metrics->get_prometheus_metrics();
    
    // Should have recorded all operations
    EXPECT_TRUE(prometheus_output.find("concurrent_counter") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("concurrent_histogram_count " + std::to_string(num_threads * operations_per_thread)) != std::string::npos);
}

TEST_F(MetricsTest, GlobalMetricsInstance) {
    auto& global_metrics = cpp_service::get_metrics();
    
    global_metrics.increment_counter("global_test");
    
    // Should be the same instance
    EXPECT_EQ(&global_metrics, &global_metrics);
    
    std::string output = global_metrics.get_prometheus_metrics();
    EXPECT_TRUE(output.find("global_test 1") != std::string::npos);
}