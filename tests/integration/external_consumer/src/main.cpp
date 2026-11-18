#include "nova/nova.h"

#include <iostream>

int main() {
    nova::sdk::Application app;

    // Initialize the SDK
    auto init_result = app.initialize();
    if (!init_result.ok) {
        std::cerr << "Failed to initialize NOVA SDK: "
                  << init_result.error.message << std::endl;
        return 1;
    }

    // Start the runtime
    if (!app.start().ok) {
        std::cerr << "Failed to start NOVA SDK" << std::endl;
        return 1;
    }

    // Get SDK version
    auto version = nova::sdk::Application::version();
    std::cout << "NOVA SDK version: " << version.major << "."
              << version.minor << "." << version.patch << std::endl;

    // Access the SDK directly
    auto sdk = app.sdk();
    if (sdk && sdk->is_initialized()) {
        std::cout << "SDK is initialized" << std::endl;
    }

    // Check runtime state
    auto& runtime = sdk->runtime();
    std::cout << "Runtime initialized: "
              << (runtime.is_initialized() ? "true" : "false") << std::endl;

    // Shutdown
    app.stop();
    app.shutdown();

    std::cout << "External consumer completed successfully" << std::endl;
    return 0;
}
