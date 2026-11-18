#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>

namespace nova::core {

enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
};

class Logger {
public:
    static Logger& instance() noexcept;

    void set_level(LogLevel level) noexcept;
    LogLevel level() const noexcept;

    void set_output(std::ostream* output) noexcept;
    std::ostream* output() const noexcept;

    template <typename... Args>
    void log(LogLevel level, std::string_view format, Args&&... args) {
        if (level < level_) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        *out_ << "[" << level_string(level) << "] " << format << "\n";
        out_->flush();
    }

    template <typename... Args>
    void trace(std::string_view format, Args&&... args) {
        log(LogLevel::Trace, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(std::string_view format, Args&&... args) {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view format, Args&&... args) {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::string_view format, Args&&... args) {
        log(LogLevel::Warning, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view format, Args&&... args) {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(std::string_view format, Args&&... args) {
        log(LogLevel::Critical, format, std::forward<Args>(args)...);
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static const char* level_string(LogLevel level) noexcept;

    LogLevel level_ = LogLevel::Info;
    std::ostream* out_ = &std::cout;
    mutable std::mutex mutex_;
};

}  // namespace nova::core
