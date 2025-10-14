#!/bin/bash

# Screenshot Setup Script for C++ Service
# This script helps you recreate all the screenshots for the README

echo "📸 C++ Service Screenshot Setup"
echo "================================"

# Function to wait for user input
wait_for_user() {
    echo "Press Enter when ready to continue..."
    read -r
}

# Function to cleanup processes
cleanup() {
    echo "🧹 Cleaning up..."
    pkill -f cpp-service 2>/dev/null
    docker stop $(docker ps -q --filter ancestor=cpp-service:latest) 2>/dev/null
}

# Set up trap for cleanup on exit
trap cleanup EXIT

echo "This script will help you recreate screenshots for your C++ service README."
echo ""
echo "📋 Screenshots to capture:"
echo "1. Interactive Demo (./demo.sh)"
echo "2. Unit Tests (ctest results)"
echo "3. Performance Metrics (hey load test)"
echo "4. Docker Container (build and run)"
echo "5. Prometheus Metrics (browser)"
echo "6. JSON Statistics (browser)"
echo ""

wait_for_user

echo "🚀 Starting screenshot recreation process..."
echo ""

# 1. Interactive Demo
echo "1️⃣  INTERACTIVE DEMO SCREENSHOT"
echo "================================"
echo "Run: ./demo.sh"
echo "📸 Take screenshot of the terminal showing the demo output"
echo ""
wait_for_user

# 2. Unit Tests
echo "2️⃣  UNIT TESTS SCREENSHOT"
echo "=========================="
echo "Running unit tests..."
ctest --test-dir build --output-on-failure
echo ""
echo "📸 Take screenshot of the test results showing '100% tests passed'"
echo ""
wait_for_user

# 3. Performance Test
echo "3️⃣  PERFORMANCE METRICS SCREENSHOT"
echo "==================================="
echo "Starting service for performance test..."
./build/cpp-service --port 8081 &
SERVICE_PID=$!
sleep 3

echo "Running performance test..."
hey -z 15s -c 100 -m POST -H "Content-Type: application/json" -d '{"readings":[12.1,11.9,12.0]}' http://localhost:8081/fuse
echo ""
echo "📸 Take screenshot of the hey performance results"
echo ""

# Clean up service
kill $SERVICE_PID 2>/dev/null
wait_for_user

# 4. Docker Container
echo "4️⃣  DOCKER CONTAINER SCREENSHOT"
echo "==============================="
echo "Building Docker image..."
docker build -t cpp-service:latest -f docker/Dockerfile .
echo ""
echo "Showing image size..."
docker images cpp-service:latest
echo ""
echo "Starting container..."
docker run --rm -p 8083:8080 cpp-service:latest &
sleep 3
echo ""
echo "📸 Take screenshot of the Docker build process and container running"
echo ""
wait_for_user

# 5. Browser Screenshots
echo "5️⃣  BROWSER SCREENSHOTS"
echo "======================="
echo "Opening browser for metrics..."
echo "URL: http://localhost:8083/metrics"
open http://localhost:8083/metrics
echo ""
echo "📸 Take screenshot of the Prometheus metrics page"
echo ""
wait_for_user

echo "Opening stats page..."
echo "URL: http://localhost:8083/stats"
open http://localhost:8083/stats
echo ""
echo "📸 Take screenshot of the JSON statistics page"
echo ""
wait_for_user

# Final cleanup
cleanup

echo "✅ Screenshot setup complete!"
echo ""
echo "📁 Save your screenshots in the screenshots/ folder with these names:"
echo "   01-interactive-demo.png"
echo "   02-unit-tests.png"
echo "   03-performance-metrics.png"
echo "   04-docker-container.png"
echo "   05-prometheus-metrics.png"
echo "   06-json-stats.png"
echo ""
echo "📝 Then update your README.md to include the screenshots!"
echo ""
echo "🎉 Your C++ service is ready for GitHub showcase!"
