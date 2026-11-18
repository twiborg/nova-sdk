#pragma once

// SDK result types.
//
// Provides SdkResult for lifecycle/configuration operations and SdkResultT<T>
// for value-returning operations. All SDK public methods use result-based
// error handling (no exceptions).
#pragma once

#include "nova/sdk/sdk_error.h"

namespace nova::sdk {

// Result of an SDK lifecycle or configuration operation.
struct SdkResult {
    bool ok = false;
    SdkError error;
};

// Result of an SDK operation that returns a value of type T.
template <typename T>
struct SdkResultT {
    bool ok = false;
    T value{};
    SdkError error;
};

}  // namespace nova::sdk
