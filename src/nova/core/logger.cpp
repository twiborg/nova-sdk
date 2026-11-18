#include "nova/core/logger.h"

#include <iostream>

namespace nova::core {

Logger& Logger::instance() noexcept {
    static Logger logger;
    return logger;
}

void Logger::set_level(LogLevel level) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::set_output(std::ostream* output) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    out_ = output;
}

std::ostream* Logger::output() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return out_;
}

const char* Logger::level_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

}  // namespace nova::core
