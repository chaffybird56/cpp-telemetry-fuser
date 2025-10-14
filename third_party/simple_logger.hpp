#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>

namespace spdlog {

enum level_enum {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,
    off = 6
};

class logger {
private:
    std::string name_;
    level_enum level_;
    
public:
    logger(const std::string& name) : name_(name), level_(info) {}
    
    void set_level(level_enum l) { level_ = l; }
    
    template<typename... Args>
    void log(level_enum l, const std::string& fmt, Args&&... args) {
        if (l < level_) return;
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::cout << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        std::cout << "." << std::setfill('0') << std::setw(3) << ms.count() << " ";
        std::cout << "[" << level_to_string(l) << "] ";
        std::cout << "[" << name_ << "] ";
        
        format_and_print(fmt, std::forward<Args>(args)...);
        std::cout << std::endl;
    }
    
    template<typename... Args>
    void trace(const std::string& fmt, Args&&... args) {
        log(trace, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        log(debug, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        log(info, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        log(warn, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        log(error, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(const std::string& fmt, Args&&... args) {
        log(critical, fmt, std::forward<Args>(args)...);
    }
    
private:
    template<typename... Args>
    void format_and_print(const std::string& fmt, Args&&... args) {
        std::ostringstream oss;
        format_impl(oss, fmt, 0, std::forward<Args>(args)...);
        std::cout << oss.str();
    }
    
    template<typename T, typename... Args>
    void format_impl(std::ostringstream& oss, const std::string& fmt, size_t pos, T&& value, Args&&... args) {
        size_t next_pos = fmt.find("{}", pos);
        if (next_pos == std::string::npos) {
            oss << fmt.substr(pos);
            return;
        }
        
        oss << fmt.substr(pos, next_pos - pos);
        oss << value;
        format_impl(oss, fmt, next_pos + 2, std::forward<Args>(args)...);
    }
    
    void format_impl(std::ostringstream& oss, const std::string& fmt, size_t pos) {
        oss << fmt.substr(pos);
    }
    
    std::string level_to_string(level_enum l) {
        switch (l) {
            case trace: return "TRACE";
            case debug: return "DEBUG";
            case info: return "INFO ";
            case warn: return "WARN ";
            case error: return "ERROR";
            case critical: return "CRIT ";
            default: return "UNKNOWN";
        }
    }
};

namespace {
    thread_local logger default_logger("cpp-service");
}

logger& get_logger() {
    return default_logger;
}

void set_level(level_enum l) {
    get_logger().set_level(l);
}

template<typename... Args>
void trace(const std::string& fmt, Args&&... args) {
    get_logger().trace(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(const std::string& fmt, Args&&... args) {
    get_logger().debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(const std::string& fmt, Args&&... args) {
    get_logger().info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(const std::string& fmt, Args&&... args) {
    get_logger().warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void error(const std::string& fmt, Args&&... args) {
    get_logger().error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void critical(const std::string& fmt, Args&&... args) {
    get_logger().critical(fmt, std::forward<Args>(args)...);
}

} // namespace spdlog

