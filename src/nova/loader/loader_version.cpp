#include "nova/loader/loader_version.h"

namespace nova::loader {

LoaderVersion loader_version() noexcept {
    return LoaderVersion{};
}

std::string loader_version_string() noexcept {
    const LoaderVersion v = loader_version();
    return std::to_string(v.major) + "." + std::to_string(v.minor) + "." +
           std::to_string(v.patch);
}

}  // namespace nova::loader
