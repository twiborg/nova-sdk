#pragma once

// Cooperative cancellation support.
//
// CancellationToken and CancellationSource allow tasks to observe cancellation
// requests and react cooperatively. No forced thread termination is ever
// performed; it is the responsibility of the task callable to check the token
// and exit promptly.
#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

namespace nova::runtime {

// Shared cancellation state.
//
// A CancellationSource owns the state and can request cancellation. Any number
// of CancellationToken instances can observe the same state. Once cancelled,
// the state remains cancelled forever.
class CancellationState : public std::enable_shared_from_this<CancellationState> {
public:
    CancellationState() = default;

    // Request cancellation. Thread-safe. Idempotent.
    void request_cancellation() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        cancelled_ = true;
    }

    // Query cancellation state. Thread-safe.
    [[nodiscard]] bool is_cancelled() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return cancelled_;
    }

private:
    mutable std::mutex mutex_;
    bool cancelled_ = false;
};

// Token that observes cancellation state. Copyable and thread-safe.
//
// A task receives a token and checks is_cancelled() periodically. When the
// associated source requests cancellation, is_cancelled() returns true.
class CancellationToken {
public:
    CancellationToken() = default;
    explicit CancellationToken(std::shared_ptr<CancellationState> state)
        : state_(std::move(state)) {}

    // Returns true if cancellation has been requested.
    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_ != nullptr && state_->is_cancelled();
    }

    // Returns true if this token is associated with a source.
    [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

    [[nodiscard]] bool operator==(const CancellationToken& other) const noexcept {
        return state_ == other.state_;
    }

    [[nodiscard]] bool operator!=(const CancellationToken& other) const noexcept {
        return state_ != other.state_;
    }

private:
    std::shared_ptr<CancellationState> state_;
};

// Source that owns cancellation state and can request cancellation.
class CancellationSource {
public:
    CancellationSource()
        : state_(std::make_shared<CancellationState>()) {}

    // Returns a token that observes this source's state.
    [[nodiscard]] CancellationToken token() const noexcept {
        return CancellationToken(state_);
    }

    // Request cancellation. All associated tokens will observe is_cancelled() == true.
    void request_cancellation() noexcept { state_->request_cancellation(); }

    // Query whether cancellation has been requested.
    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_->is_cancelled();
    }

private:
    std::shared_ptr<CancellationState> state_;
};

// A task that has been cancelled cooperatively.
class CancellationException {
public:
    explicit CancellationException(std::string message)
        : message_(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept { return message_.c_str(); }

private:
    std::string message_;
};

}  // namespace nova::runtime
