#include <gtest/gtest.h>
#include "service.hpp"

class ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = std::make_unique<cpp_service::Service>();
    }
    
    void TearDown() override {
        service.reset();
    }
    
    std::unique_ptr<cpp_service::Service> service;
};

TEST_F(ServiceTest, HealthCheck) {
    EXPECT_EQ(service->health_check(), "ok");
}

TEST_F(ServiceTest, FuseReadingsBasic) {
    std::vector<double> readings = {10.0, 11.0, 12.0, 13.0, 14.0};
    double result = service->fuse_readings(readings);
    
    EXPECT_GT(result, 0.0);
    EXPECT_NEAR(result, 12.0, 1.0);  // Should be close to the middle values
}

TEST_F(ServiceTest, FuseReadingsWithOutliers) {
    std::vector<double> readings = {10.0, 11.0, 12.0, 13.0, 100.0};  // 100 is an outlier
    double result = service->fuse_readings(readings);
    
    EXPECT_GT(result, 0.0);
    EXPECT_LT(result, 50.0);  // Should not be influenced by the outlier
}

TEST_F(ServiceTest, FuseReadingsEmpty) {
    std::vector<double> readings;
    double result = service->fuse_readings(readings);
    
    EXPECT_EQ(result, 0.0);
}

TEST_F(ServiceTest, FuseReadingsSingleValue) {
    std::vector<double> readings = {42.5};
    double result = service->fuse_readings(readings);
    
    EXPECT_EQ(result, 42.5);
}

TEST_F(ServiceTest, FuseReadingsTwoValues) {
    std::vector<double> readings = {10.0, 20.0};
    double result = service->fuse_readings(readings);
    
    EXPECT_GT(result, 10.0);
    EXPECT_LT(result, 20.0);
}

TEST_F(ServiceTest, ConfigurationManagement) {
    // Test default configuration
    std::string default_config = service->get_config();
    EXPECT_FALSE(default_config.empty());
    
    // Should contain expected fields
    EXPECT_TRUE(default_config.find("outlier_threshold") != std::string::npos);
    EXPECT_TRUE(default_config.find("min_confidence") != std::string::npos);
    EXPECT_TRUE(default_config.find("enable_outlier_detection") != std::string::npos);
}

TEST_F(ServiceTest, StatisticsTracking) {
    // Initial stats should be zero
    auto stats = service->get_stats();
    EXPECT_EQ(stats.total_requests, 0);
    EXPECT_EQ(stats.successful_requests, 0);
    EXPECT_EQ(stats.failed_requests, 0);
    EXPECT_EQ(stats.average_fused_value, 0.0);
    
    // Perform some operations
    std::vector<double> readings = {10.0, 11.0, 12.0};
    service->fuse_readings(readings);
    service->fuse_readings(readings);
    
    stats = service->get_stats();
    EXPECT_EQ(stats.total_requests, 2);
    EXPECT_EQ(stats.successful_requests, 2);
    EXPECT_EQ(stats.failed_requests, 0);
    EXPECT_GT(stats.average_fused_value, 0.0);
}

TEST_F(ServiceTest, StatisticsReset) {
    // Perform operations
    std::vector<double> readings = {10.0, 11.0, 12.0};
    service->fuse_readings(readings);
    
    // Verify stats are updated
    auto stats = service->get_stats();
    EXPECT_EQ(stats.total_requests, 1);
    
    // Reset stats
    service->reset_stats();
    
    // Verify stats are reset
    stats = service->get_stats();
    EXPECT_EQ(stats.total_requests, 0);
    EXPECT_EQ(stats.successful_requests, 0);
    EXPECT_EQ(stats.failed_requests, 0);
    EXPECT_EQ(stats.average_fused_value, 0.0);
}

TEST_F(ServiceTest, LargeDataset) {
    // Test with larger dataset
    std::vector<double> readings;
    for (int i = 0; i < 100; ++i) {
        readings.push_back(10.0 + (i % 10));  // Values from 10.0 to 19.0
    }
    
    double result = service->fuse_readings(readings);
    
    EXPECT_GT(result, 10.0);
    EXPECT_LT(result, 20.0);
    EXPECT_NEAR(result, 14.5, 1.0);  // Should be close to middle of range
}

TEST_F(ServiceTest, IdenticalReadings) {
    // Test with all identical readings
    std::vector<double> readings(10, 42.0);
    double result = service->fuse_readings(readings);
    
    EXPECT_EQ(result, 42.0);
}

TEST_F(ServiceTest, UptimeTracking) {
    auto stats = service->get_stats();
    auto now = std::chrono::steady_clock::now();
    
    // Uptime should be reasonable (not negative, not too large)
    auto uptime = now - stats.start_time;
    EXPECT_GE(uptime.count(), 0);
    
    // Should be less than a minute for this test
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime);
    EXPECT_LT(uptime_seconds.count(), 60);
}