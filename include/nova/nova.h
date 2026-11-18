#pragma once

// NOVA SDK umbrella header.
//
// This single header provides access to the complete public NOVA SDK API.
// Including this header is equivalent to including all public SDK headers
// individually. It is self-contained and does not expose any internal
// implementation details.
//
// Example usage:
//
//     #include "nova/nova.h"
//
//     nova::sdk::Application app;
//     auto result = app.initialize();
//     if (result.ok) {
//         app.start();
//         // ...
//         app.shutdown();
//     }

#include "nova/sdk/application.h"
#include "nova/sdk/application_context.h"
#include "nova/sdk/sdk.h"
#include "nova/sdk/sdk_config.h"
#include "nova/sdk/sdk_error.h"
#include "nova/sdk/sdk_result.h"
#include "nova/sdk/sdk_version.h"
