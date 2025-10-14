#!/bin/bash

# 🚀 C++ Service Demo Script
# This script demonstrates the complete functionality of the production C++ service

set -e

echo "🚀 Starting C++ Service Demo"
echo "================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}$1${NC}"
}

# Check if service is built
if [ ! -f "./build/cpp-service" ]; then
    print_error "Service not built. Please run: cmake --build build"
    exit 1
fi

# Kill any existing service
print_status "Cleaning up any existing service..."
pkill -f cpp-service || true
sleep 1

# Start the service
print_header "🏗️  Starting C++ Service..."
./build/cpp-service --port 8080 &
SERVICE_PID=$!
sleep 3

# Check if service started successfully
if ! curl -s http://localhost:8080/health > /dev/null; then
    print_error "Failed to start service. Check logs."
    kill $SERVICE_PID 2>/dev/null || true
    exit 1
fi

print_status "✅ Service started successfully (PID: $SERVICE_PID)"

# Demo endpoints
print_header "📡 Testing HTTP Endpoints..."

echo ""
print_status "1. Health Check:"
curl -s http://localhost:8080/health | jq . || curl -s http://localhost:8080/health

echo ""
print_status "2. Data Fusion (Normal Data):"
curl -s -X POST http://localhost:8080/fuse \
  -H 'Content-Type: application/json' \
  -d '{"readings":[12.1,11.9,12.0,12.2]}' | jq . || \
curl -s -X POST http://localhost:8080/fuse \
  -H 'Content-Type: application/json' \
  -d '{"readings":[12.1,11.9,12.0,12.2]}'

echo ""
print_status "3. Data Fusion (With Outliers):"
curl -s -X POST http://localhost:8080/fuse \
  -H 'Content-Type: application/json' \
  -d '{"readings":[12.1,11.9,12.0,12.2,50.0,1.0]}' | jq . || \
curl -s -X POST http://localhost:8080/fuse \
  -H 'Content-Type: application/json' \
  -d '{"readings":[12.1,11.9,12.0,12.2,50.0,1.0]}'

echo ""
print_status "4. Prometheus Metrics:"
curl -s http://localhost:8080/metrics | head -20

echo ""
print_status "5. JSON Statistics:"
curl -s http://localhost:8080/stats | jq . || curl -s http://localhost:8080/stats

echo ""
print_status "6. Configuration:"
curl -s http://localhost:8080/config | jq . || curl -s http://localhost:8080/config

# Load testing demo
print_header "⚡ Load Testing Demo..."

if command -v hey >/dev/null 2>&1; then
    print_status "Running load test with hey (10 seconds, 50 concurrent users):"
    hey -z 10s -c 50 -m POST \
      -H "Content-Type: application/json" \
      -d '{"readings":[12.1,11.9,12.0]}' \
      http://localhost:8080/fuse
else
    print_warning "hey not installed. Install with: brew install hey"
    print_status "Running simple load test with curl..."
    
    # Simple load test
    for i in {1..20}; do
        curl -s -X POST http://localhost:8080/fuse \
          -H 'Content-Type: application/json' \
          -d '{"readings":[12.1,11.9,12.0]}' > /dev/null &
    done
    wait
    print_status "✅ Completed 20 concurrent requests"
fi

# Show final metrics
print_header "📊 Final Metrics:"
curl -s http://localhost:8080/stats | jq '.data | {total_requests, successful_requests, failed_requests, uptime_seconds}' || \
curl -s http://localhost:8080/stats

# Cleanup
print_header "🧹 Cleanup..."
print_status "Stopping service..."
kill $SERVICE_PID 2>/dev/null || true
sleep 2

# Check if service stopped
if kill -0 $SERVICE_PID 2>/dev/null; then
    print_warning "Service still running, force killing..."
    kill -9 $SERVICE_PID 2>/dev/null || true
fi

print_status "✅ Demo completed successfully!"

echo ""
print_header "🎯 Key Takeaways:"
echo "• Production-ready C++17 microservice"
echo "• Real-time metrics and monitoring"
echo "• High-performance data fusion"
echo "• Comprehensive error handling"
echo "• Docker-ready containerization"
echo "• Complete CI/CD pipeline"

echo ""
print_header "🚀 Next Steps:"
echo "• Review PERFORMANCE_SHOWCASE.md for detailed benchmarks"
echo "• Run tests: ctest --test-dir build --output-on-failure"
echo "• Build Docker: docker build -t cpp-service:latest -f docker/Dockerfile ."
echo "• Check CI: .github/workflows/ci.yml"

