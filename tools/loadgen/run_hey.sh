#!/bin/bash

# Load testing script using hey (HTTP load generator)
# Usage: ./run_hey.sh [URL] [DURATION] [CONCURRENCY]

set -e

# Default values
URL=${1:-"http://localhost:8080"}
DURATION=${2:-"30s"}
CONCURRENCY=${3:-"100"}

# Check if hey is installed
if ! command -v hey &> /dev/null; then
    echo "Error: hey is not installed. Install it with:"
    echo "  brew install hey  # macOS"
    echo "  go install github.com/rakyll/hey@latest  # Linux"
    exit 1
fi

echo "Starting load test with hey..."
echo "URL: $URL"
echo "Duration: $DURATION"
echo "Concurrency: $CONCURRENCY"
echo ""

# Test health endpoint
echo "=== Health Endpoint Test ==="
hey -z $DURATION -c $CONCURRENCY "$URL/health"

echo ""
echo "=== Fusion Endpoint Test ==="
# Test fusion endpoint with JSON payload
hey -z $DURATION -c $CONCURRENCY \
    -m POST \
    -H "Content-Type: application/json" \
    -d '{"readings":[12.1,11.9,12.0,12.2,11.8]}' \
    "$URL/fuse"

echo ""
echo "=== Metrics Endpoint Test ==="
hey -z $DURATION -c $CONCURRENCY "$URL/metrics"

echo ""
echo "Load testing completed!"

