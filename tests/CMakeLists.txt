# Tests CMakeLists.txt

# Find GoogleTest
include(FetchGTest)

# Service tests
add_executable(service_tests
    service_tests.cpp
)

target_link_libraries(service_tests
    cpp-service-lib
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(service_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Metrics tests
add_executable(metrics_tests
    metrics_tests.cpp
)

target_link_libraries(metrics_tests
    cpp-service-lib
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(metrics_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Register tests
include(GoogleTest)
gtest_discover_tests(service_tests)
gtest_discover_tests(metrics_tests)