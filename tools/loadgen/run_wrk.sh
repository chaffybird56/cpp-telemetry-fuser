#!/bin/bash

# Load testing script using wrk (HTTP benchmarking tool)
# Usage: ./run_wrk.sh [URL] [DURATION] [THREADS] [CONNECTIONS]

set -e

# Default values
URL=${1:-"http://localhost:8080"}
DURATION=${2:-"30s"}
THREADS=${3:-"4"}
CONNECTIONS=${4:-"100"}

# Check if wrk is installed
if ! command -v wrk &> /dev/null; then
    echo "Error: wrk is not installed. Install it with:"
    echo "  brew install wrk  # macOS"
    echo "  sudo apt-get install wrk  # Ubuntu/Debian"
    exit 1
fi

echo "Starting load test with wrk..."
echo "URL: $URL"
echo "Duration: $DURATION"
echo "Threads: $THREADS"
echo "Connections: $CONNECTIONS"
echo ""

# Test health endpoint
echo "=== Health Endpoint Test ==="
wrk -t$THREADS -c$CONNECTIONS -d$DURATION "$URL/health"

echo ""
echo "=== Fusion Endpoint Test ==="
# Create a Lua script for POST requests
cat > /tmp/fusion_script.lua << 'EOF'
wrk.method = "POST"
wrk.body = '{"readings":[12.1,11.9,12.0,12.2,11.8]}'
wrk.headers["Content-Type"] = "application/json"
EOF

wrk -t$THREADS -c$CONNECTIONS -d$DURATION -s /tmp/fusion_script.lua "$URL/fuse"

echo ""
echo "=== Metrics Endpoint Test ==="
wrk -t$THREADS -c$CONNECTIONS -d$DURATION "$URL/metrics"

# Clean up
rm -f /tmp/fusion_script.lua

echo ""
echo "Load testing completed!"

