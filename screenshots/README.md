# 📸 Screenshots for GitHub README

This folder contains screenshots to showcase the production C++ service.

## 📋 Screenshot Guide

### **1. Interactive Demo Screenshot**
**File:** `01-interactive-demo.png`
**What to capture:** Terminal showing `./demo.sh` script running
**Key content to show:**
```bash
🚀 Starting C++ Service Demo
================================
🏗️  Starting C++ Service...
✅ Service started successfully (PID: 51862)
📡 Testing HTTP Endpoints...

[INFO] 1. Health Check:
{
  "status": "success",
  "data": {
    "status": "ok",
    "version": "0.1.0"
  }
}

[INFO] 2. Data Fusion (Normal Data):
{
  "status": "success",
  "data": {
    "fused_value": "12.050000",
    "input_count": "4"
  }
}

[INFO] 3. Data Fusion (With Outliers):
{
  "status": "success",
  "data": {
    "fused_value": "12.050000",
    "input_count": "6"
  }
}
```

### **2. Unit Tests Screenshot**
**File:** `02-unit-tests.png`
**What to capture:** Terminal showing `ctest` results
**Key content to show:**
```bash
Test project /Users/ahmadali/Downloads/cpp-service/build
      Start  1: ServiceTest.HealthCheck ................   Passed    0.00 sec
      Start  2: ServiceTest.FuseReadingsBasic ..........   Passed    0.00 sec
      Start  3: ServiceTest.FuseReadingsWithOutliers ...   Passed    0.00 sec
      ...
      Start 24: MetricsTest.GlobalMetricsInstance ......   Passed    0.00 sec

100% tests passed, 0 tests failed out of 24
Total Test time (real) =   0.08 sec
```

### **3. Performance Metrics Screenshot**
**File:** `03-performance-metrics.png`
**What to capture:** Terminal showing `hey` load testing results
**Key content to show:**
```bash
hey -z 15s -c 100 -m POST http://localhost:8081/fuse

Summary:
  Total:	20.9297 secs
  Slowest:	5.9235 secs
  Fastest:	0.0002 secs
  Average:	0.0044 secs
  Requests/sec:	13273.0273

Response time histogram:
  0.000 [1]	|
  0.592 [277606]	|■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  1.185 [44]	|

Latency distribution:
  10% in 0.0006 secs
  25% in 0.0008 secs
  50% in 0.0009 secs
  75% in 0.0011 secs
  90% in 0.0014 secs
  95% in 0.0307 secs
  99% in 0.0612 secs

Status code distribution:
  [200]	277689 responses
```

### **4. Docker Container Screenshot**
**File:** `04-docker-container.png`
**What to capture:** Terminal showing Docker build and run
**Key content to show:**
```bash
docker build -t cpp-service:latest -f docker/Dockerfile .

[+] Building 45.2s (16/16) FINISHED
 => [builder 6/6] RUN cmake --build build --config Release
 => [runtime 6/6] RUN chown cppservice:cppservice /app/cpp-service

docker images cpp-service:latest
REPOSITORY    TAG       IMAGE ID       CREATED        SIZE
cpp-service   latest    9648eea3f0fd   37 hours ago   126MB

docker run --rm -p 8083:8080 cpp-service:latest
Starting C++ service on port 8080
HTTP Server running on port 8080
Server listening on port 8080
```

### **5. Prometheus Metrics Screenshot**
**File:** `05-prometheus-metrics.png`
**What to capture:** Browser showing `/metrics` endpoint
**URL:** `http://localhost:8083/metrics`
**Key content to show:**
```
# HELP requests_total Total count
# TYPE requests_total counter
requests_total{endpoint="/health"} 2
requests_total{endpoint="/metrics"} 1
# HELP request_duration_ms Request duration histogram
# TYPE request_duration_ms histogram
request_duration_ms_bucket{endpoint="/health",le="1.000000"} 2
request_duration_ms_bucket{endpoint="/health",le="5.000000"} 2
request_duration_ms_bucket{endpoint="/health",le="10.000000"} 2
```

### **6. JSON Statistics Screenshot**
**File:** `06-json-stats.png`
**What to capture:** Browser showing `/stats` endpoint
**URL:** `http://localhost:8083/stats`
**Key content to show:**
```json
{
  "status": "success",
  "data": {
    "average_fused_value": "0.000000",
    "failed_requests": "0",
    "metrics": "{ ... }",
    "successful_requests": "0",
    "total_requests": "0",
    "uptime_seconds": "24"
  }
}
```

## 🎯 Screenshot Tips

### **Terminal Screenshots:**
- Use a clean, professional terminal theme
- Ensure good contrast and readability
- Capture the full terminal window
- Include the command prompt to show it's real

### **Browser Screenshots:**
- Use a clean browser window (no bookmarks bar)
- Ensure good font rendering
- Capture the full content area
- Consider using a dark theme for contrast

### **File Naming Convention:**
- Use descriptive names with numbers for ordering
- Include the main feature being demonstrated
- Keep names short and clear

## 📝 README Integration

Add these screenshots to your README.md:

```markdown
## 🚀 Demo & Testing

### Interactive Demo
![Interactive Demo](screenshots/01-interactive-demo.png)

### Unit Tests
![Unit Tests](screenshots/02-unit-tests.png)

### Performance Metrics
![Performance Metrics](screenshots/03-performance-metrics.png)

### Docker Container
![Docker Container](screenshots/04-docker-container.png)

### Prometheus Metrics
![Prometheus Metrics](screenshots/05-prometheus-metrics.png)

### JSON Statistics
![JSON Statistics](screenshots/06-json-stats.png)
```

## 🚀 Quick Commands to Recreate Screenshots

If you need to recreate any screenshots:

```bash
# 1. Interactive Demo
./demo.sh

# 2. Unit Tests
ctest --test-dir build --output-on-failure

# 3. Performance Test
./build/cpp-service --port 8081 &
hey -z 15s -c 100 -m POST -H "Content-Type: application/json" -d '{"readings":[12.1,11.9,12.0]}' http://localhost:8081/fuse

# 4. Docker Build & Run
docker build -t cpp-service:latest -f docker/Dockerfile .
docker run --rm -p 8083:8080 cpp-service:latest

# 5. Browser - Metrics
open http://localhost:8083/metrics

# 6. Browser - Stats
open http://localhost:8083/stats
```
