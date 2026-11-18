#include "nova/core/config.h"
#include "nova/core/init.h"
#include "nova/core/logger.h"

#include <iostream>

int main() {
    nova::core::Logger::instance().set_level(nova::core::LogLevel::Trace);

    nova::core::Logger::instance().info("NOVA Core starting");

    nova::core::Core core;
    if (!core.initialize()) {
        nova::core::Logger::instance().error("Failed to initialize NOVA Core");
        return 1;
    }

    nova::core::Config& config = nova::core::Config::instance();
    config.set("app.name", std::string("NOVA"));
    config.set("app.version", std::string("0.1.0"));
    config.set("debug.enabled", true);

    nova::core::Logger::instance().info("Configuration: " + config.to_json());

    core.shutdown();
    nova::core::Logger::instance().info("NOVA Core stopped");

    return 0;
}
