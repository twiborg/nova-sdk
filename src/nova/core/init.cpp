#include "nova/core/init.h"

#include "nova/core/logger.h"

namespace nova::core {

bool Core::initialize() {
    if (initialized_.load(std::memory_order_acquire)) {
        Logger::instance().warning("NOVA Core is already initialized");
        return false;
    }
    Logger::instance().info("Initializing NOVA Core");
    initialized_.store(true, std::memory_order_release);
    return true;
}

void Core::shutdown() {
    if (!initialized_.load(std::memory_order_acquire)) {
        return;
    }
    Logger::instance().info("Shutting down NOVA Core");
    initialized_.store(false, std::memory_order_release);
}

}  // namespace nova::core
