#pragma once

#include "nova/core/error.h"

#include <atomic>
#include <string>

namespace nova::core {

class Core {
public:
    Core() = default;
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

    [[nodiscard]] bool initialize();
    void shutdown();

    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }

private:
    std::atomic<bool> initialized_{false};
};

}  // namespace nova::core
