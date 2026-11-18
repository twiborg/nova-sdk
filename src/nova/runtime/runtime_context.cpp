#include "nova/runtime/runtime_context.h"

namespace nova::runtime {

RuntimeContext::RuntimeContext(std::string name,
                               std::uint64_t id,
                               std::string version,
                               RuntimeState state,
                               RuntimeConfig config,
                               std::string metadata,
                               std::chrono::system_clock::time_point creation_time)
    : name_(std::move(name)),
      id_(id),
      version_(std::move(version)),
      state_(state),
      config_(std::move(config)),
      metadata_(std::move(metadata)),
      creation_time_(creation_time) {}

}  // namespace nova::runtime
