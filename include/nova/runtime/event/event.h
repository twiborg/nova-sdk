#pragma once

// Event abstraction.
//
// Supports typed event payloads via std::any while keeping the dispatcher
// type-erased and decoupled from concrete event types. Supports optional
// filtering via a predicate stored with the subscription.
#pragma once

#include <any>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace nova::runtime {

// Unique identifier for an event type/instance.
using EventId = std::uint64_t;

// Unique identifier for a subscription.
using SubscriptionId = std::uint64_t;

// A single event dispatched to subscribers.
struct Event {
    EventId id = 0;
    std::string type;                      // event type name (e.g. "task.completed")
    std::any payload;                      // optional typed payload
    std::chrono::system_clock::time_point timestamp;
};

// Metadata associated with a subscription.
struct SubscriptionMetadata {
    SubscriptionId id = 0;
    std::string type;                      // subscribed event type
    std::uint64_t dispatch_count = 0;      // number of events delivered
};

// Callback invoked when a matching event is dispatched.
using EventHandler = std::function<void(const Event&)>;

// Optional filter predicate. Return true to deliver the event, false to skip.
using EventFilter = std::function<bool(const Event&)>;

}  // namespace nova::runtime
