#include "nova/nova.h"

#include <iostream>

int main() {
    nova::sdk::Application app;

    std::cout << "Initializing NOVA SDK..." << std::endl;

    auto init_result = app.initialize();
    if (!init_result.ok) {
        std::cerr << "Failed to initialize NOVA SDK: "
                  << init_result.error.message << std::endl;
        return 1;
    }

    std::cout << "SDK initialized successfully" << std::endl;

    // Access SDK version
    auto version = nova::sdk::Application::version();
    std::cout << "SDK version: " << version.major << "."
              << version.minor << "." << version.patch << std::endl;

    // Start runtime
    if (!app.start().ok) {
        std::cerr << "Failed to start NOVA SDK" << std::endl;
        return 1;
    }

    std::cout << "SDK started successfully" << std::endl;

    // Access runtime
    auto& runtime = app.sdk()->runtime();
    std::cout << "Runtime initialized: "
              << (runtime.is_initialized() ? "true" : "false") << std::endl;

    // Shutdown
    app.stop();
    app.shutdown();

    std::cout << "External example completed successfully" << std::endl;
    return 0;
}
