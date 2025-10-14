cmake_minimum_required(VERSION 3.16)
project(cpp-service 
    VERSION 0.1.0
    DESCRIPTION "Production-style C++17 service with tests, sanitizers, metrics, and Docker"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type defaults to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include our custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Set up warnings and sanitizers
include(Warnings)

# Options
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior sanitizers" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_TESTS "Build tests" ON)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
endif()

# Add sanitizer flags if enabled
if(ENABLE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

# Add coverage flags if enabled
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Include directories
include_directories(include)

# Fetch GoogleTest for testing
if(BUILD_TESTS)
    include(FetchGTest)
endif()

# Create the service library
set(SERVICE_SOURCES
    src/service.cpp
    src/metrics.cpp
    src/http_server.cpp
)

set(SERVICE_HEADERS
    include/service.hpp
    include/metrics.hpp
    include/http_server.hpp
)

add_library(cpp-service-lib STATIC ${SERVICE_SOURCES} ${SERVICE_HEADERS})
target_include_directories(cpp-service-lib PUBLIC include)

# Create the main executable
add_executable(cpp-service src/main.cpp)
target_link_libraries(cpp-service cpp-service-lib)

# Link pthread for threading support
find_package(Threads REQUIRED)
target_link_libraries(cpp-service Threads::Threads)

# Install targets
install(TARGETS cpp-service DESTINATION bin)
install(TARGETS cpp-service-lib DESTINATION lib)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Create a config.h file with version info
configure_file(
    "${CMAKE_SOURCE_DIR}/include/config.h.in"
    "${CMAKE_BINARY_DIR}/include/config.h"
)
target_include_directories(cpp-service-lib PUBLIC "${CMAKE_BINARY_DIR}/include")

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "Coverage: ${ENABLE_COVERAGE}")
message(STATUS "Tests: ${BUILD_TESTS}")