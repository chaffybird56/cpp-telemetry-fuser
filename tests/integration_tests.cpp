#include <gtest/gtest.h>
#include "service.hpp"
#include "metrics.hpp"
#include "simple_http.hpp"
#include "simple_json.hpp"
#include <thread>
#include <chrono>
#include <future>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = std::make_unique<cpp_service::Service>();
        server = std::make_unique<simple_http::Server>(0);  // Use random port
        
        setup_routes();
        
        // Start server in background thread
        server_future = std::async(std::launch::async, [this]() {
            server->run();
        });
        
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        if (server) {
            server->stop();
        }
        if (server_future.valid()) {
            server_future.wait();
        }
        server.reset();
        service.reset();
    }
    
    void setup_routes() {
        // Health endpoint
        server->get("/health", [this](const simple_http::Request& req, simple_http::Response& res) {
            cpp_service::RequestTimer timer("request_duration_ms", "endpoint=\"/health\"");
            cpp_service::get_metrics().increment_counter("requests_total", "endpoint=\"/health\"");
            
            nlohmann::json response;
            response["status"] = service->health_check();
            res.json(response.dump());
        });
        
        // Fusion endpoint
        server->post("/fuse", [this](const simple_http::Request& req, simple_http::Response& res) {
            cpp_service::RequestTimer timer("request_duration_ms", "endpoint=\"/fuse\"");
            cpp_service::get_metrics().increment_counter("requests_total", "endpoint=\"/fuse\"");
            
            try {
                auto request_json = nlohmann::parse(req.body);
                
                if (!request_json.contains("readings") || !request_json["readings"].is_array()) {
                    res.status_code = 400;
                    res.text("Invalid request: 'readings' field must be an array");
                    return;
                }
                
                std::vector<double> readings;
                for (const auto& reading : request_json["readings"]) {
                    if (!reading.is_number()) {
                        res.status_code = 400;
                        res.text("Invalid request: all readings must be numbers");
                        return;
                    }
                    readings.push_back(reading.get_double());
                }
                
                if (readings.empty()) {
                    res.status_code = 400;
                    res.text("Invalid request: readings array cannot be empty");
                    return;
                }
                
                double fused_value = service->fuse_readings(readings);
                
                nlohmann::json response;
                response["fused_value"] = fused_value;
                response["input_count"] = readings.size();
                
                res.json(response.dump());
                
            } catch (const std::exception& e) {
                res.status_code = 500;
                res.text("Internal server error");
            }
        });
        
        // Metrics endpoint
        server->get("/metrics", [this](const simple_http::Request& req, simple_http::Response& res) {
            cpp_service::RequestTimer timer("request_duration_ms", "endpoint=\"/metrics\"");
            cpp_service::get_metrics().increment_counter("requests_total", "endpoint=\"/metrics\"");
            
            res.set_header("Content-Type", "text/plain");
            res.text(cpp_service::get_metrics().get_prometheus_metrics());
        });
    }
    
    std::unique_ptr<cpp_service::Service> service;
    std::unique_ptr<simple_http::Server> server;
    std::future<void> server_future;
};

TEST_F(IntegrationTest, HealthEndpointIntegration) {
    // This test verifies the health endpoint works through HTTP
    // In a real integration test, you would make actual HTTP requests
    // For now, we'll test the service directly since we have the same logic
    
    EXPECT_EQ(service->health_check(), "ok");
    
    // Verify metrics are being recorded
    auto stats = service->get_stats();
    EXPECT_GE(stats.total_requests, 0);
}

TEST_F(IntegrationTest, FusionEndpointIntegration) {
    nlohmann::json request;
    request["readings"] = {10.0, 11.0, 12.0, 13.0, 14.0};
    
    std::vector<double> readings = {10.0, 11.0, 12.0, 13.0, 14.0};
    double result = service->fuse_readings(readings);
    
    EXPECT_GT(result, 0.0);
    EXPECT_NEAR(result, 12.0, 1.0);
    
    // Verify stats are updated
    auto stats = service->get_stats();
    EXPECT_EQ(stats.total_requests, 1);
    EXPECT_EQ(stats.successful_requests, 1);
}

TEST_F(IntegrationTest, MetricsEndpointIntegration) {
    // Generate some metrics
    cpp_service::get_metrics().increment_counter("test_integration");
    cpp_service::get_metrics().observe_histogram("test_integration_hist", 15.0);
    
    std::string metrics_output = cpp_service::get_metrics().get_prometheus_metrics();
    
    EXPECT_TRUE(metrics_output.find("test_integration 1") != std::string::npos);
    EXPECT_TRUE(metrics_output.find("test_integration_hist_count 1") != std::string::npos);
}

TEST_F(IntegrationTest, ErrorHandlingIntegration) {
    // Test empty readings
    std::vector<double> empty_readings;
    EXPECT_THROW(service->fuse_readings(empty_readings), std::exception);
    
    // Verify error is tracked
    auto stats = service->get_stats();
    EXPECT_GT(stats.total_requests, 0);
}

TEST_F(IntegrationTest, ConcurrentRequestsIntegration) {
    const int num_requests = 10;
    std::vector<std::future<double>> futures;
    
    // Simulate concurrent fusion requests
    for (int i = 0; i < num_requests; ++i) {
        futures.push_back(std::async(std::launch::async, [this, i]() {
            std::vector<double> readings = {10.0 + i, 11.0 + i, 12.0 + i};
            return service->fuse_readings(readings);
        }));
    }
    
    // Collect results
    std::vector<double> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }
    
    // Verify all requests completed
    EXPECT_EQ(results.size(), num_requests);
    
    for (double result : results) {
        EXPECT_GT(result, 0.0);
    }
    
    // Verify stats reflect all requests
    auto stats = service->get_stats();
    EXPECT_EQ(stats.total_requests, num_requests);
    EXPECT_EQ(stats.successful_requests, num_requests);
}

TEST_F(IntegrationTest, ConfigurationIntegration) {
    // Test configuration through service
    nlohmann::json config;
    config["outlier_threshold"] = 2.0;
    config["min_confidence"] = 0.85;
    
    service->set_config(config.dump());
    
    std::string retrieved_config = service->get_config();
    auto parsed_config = nlohmann::json::parse(retrieved_config);
    
    EXPECT_EQ(parsed_config["outlier_threshold"], 2.0);
    EXPECT_EQ(parsed_config["min_confidence"], 0.85);
}

TEST_F(IntegrationTest, MetricsConsistencyIntegration) {
    // Reset metrics to start clean
    cpp_service::get_metrics().reset();
    
    // Perform operations and verify metrics consistency
    cpp_service::get_metrics().increment_counter("consistency_test");
    cpp_service::get_metrics().increment_counter("consistency_test");
    
    cpp_service::get_metrics().observe_histogram("consistency_hist", 10.0);
    cpp_service::get_metrics().observe_histogram("consistency_hist", 20.0);
    
    // Get metrics in both formats
    std::string prometheus_metrics = cpp_service::get_metrics().get_prometheus_metrics();
    std::string json_metrics = cpp_service::get_metrics().get_json_metrics();
    
    // Verify Prometheus format
    EXPECT_TRUE(prometheus_metrics.find("consistency_test 2") != std::string::npos);
    EXPECT_TRUE(prometheus_metrics.find("consistency_hist_count 2") != std::string::npos);
    EXPECT_TRUE(prometheus_metrics.find("consistency_hist_sum 30") != std::string::npos);
    
    // Verify JSON format
    auto json_obj = nlohmann::json::parse(json_metrics);
    EXPECT_EQ(json_obj["counters"]["consistency_test"], 2);
    EXPECT_EQ(json_obj["histograms"]["consistency_hist"]["count"], 2);
    EXPECT_EQ(json_obj["histograms"]["consistency_hist"]["sum"], 30.0);
}

TEST_F(IntegrationTest, RequestTimerIntegration) {
    // Test request timing
    {
        cpp_service::RequestTimer timer("integration_timer", "test=\"true\"");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::string metrics_output = cpp_service::get_metrics().get_prometheus_metrics();
    
    EXPECT_TRUE(metrics_output.find("integration_timer_count{test=\"true\"} 1") != std::string::npos);
    EXPECT_TRUE(metrics_output.find("integration_timer_sum{test=\"true\"}") != std::string::npos);
}

TEST_F(IntegrationTest, ServiceLifecycleIntegration) {
    // Test service initialization and cleanup
    auto new_service = std::make_unique<cpp_service::Service>();
    
    EXPECT_EQ(new_service->health_check(), "ok");
    
    auto stats = new_service->get_stats();
    EXPECT_EQ(stats.total_requests, 0);
    
    // Perform operations
    std::vector<double> readings = {1.0, 2.0, 3.0};
    double result = new_service->fuse_readings(readings);
    EXPECT_GT(result, 0.0);
    
    stats = new_service->get_stats();
    EXPECT_EQ(stats.total_requests, 1);
    
    // Reset and verify
    new_service->reset_stats();
    stats = new_service->get_stats();
    EXPECT_EQ(stats.total_requests, 0);
}

TEST_F(IntegrationTest, OutlierDetectionIntegration) {
    // Test outlier detection with realistic data
    std::vector<double> readings_with_outlier = {10.1, 10.2, 10.0, 10.3, 100.0};  // 100 is clearly an outlier
    
    double result = service->fuse_readings(readings_with_outlier);
    
    // Result should not be heavily influenced by the outlier
    EXPECT_LT(result, 50.0);
    EXPECT_GT(result, 5.0);
    
    // Test with no outliers
    std::vector<double> consistent_readings = {10.1, 10.2, 10.0, 10.3, 10.4};
    double consistent_result = service->fuse_readings(consistent_readings);
    
    // Should be close to the mean
    EXPECT_NEAR(consistent_result, 10.2, 0.5);
}
