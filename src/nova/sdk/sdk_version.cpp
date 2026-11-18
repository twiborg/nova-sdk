#include "nova/sdk/sdk_version.h"

namespace nova::sdk {

SdkVersion sdk_version() noexcept {
    return SdkVersion{
        NOVA_VERSION_MAJOR,
        NOVA_VERSION_MINOR,
        NOVA_VERSION_PATCH,
    };
}

std::string sdk_version_string() noexcept {
    return NOVA_VERSION_STRING;
}

}  // namespace nova::sdk
