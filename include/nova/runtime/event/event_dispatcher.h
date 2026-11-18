#pragma once

// Event dispatcher.
//
// Delivers events to subscribed handlers. Subscription management is
// thread-safe; callbacks are invoked outside the internal lock so a slow or
// throwing handler cannot block or corrupt the dispatcher. Supports event
// filtering, subscription metadata, nested dispatch, and dispatch statistics.
#pragma once

#include "nova/runtime/event/event.h"

#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nova::runtime {

// Dispatches events to registered handlers.
class EventDispatcher {
public:
    EventDispatcher();
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) noexcept = default;
    EventDispatcher& operator=(EventDispatcher&&) noexcept = default;
    ~EventDispatcher();

    // Subscribe a handler to events of the given type. Returns a subscription
    // id that can be used with unsubscribe(). An optional filter predicate
    // can be supplied; only events for which the filter returns true are
    // delivered.
    [[nodiscard]] SubscriptionId subscribe(std::string type,
                                           EventHandler handler,
                                           EventFilter filter = {});

    // Unsubscribe a previously registered subscription. Returns true if the
    // subscription existed and was removed.
    [[nodiscard]] bool unsubscribe(SubscriptionId subscription_id);

    // Dispatch an event to all matching subscribers. Handlers are invoked
    // outside the internal lock. Exceptions thrown by a handler are caught
    // and do not prevent other handlers from running. Returns the number of
    // handlers invoked. Nested dispatch (dispatching from within a handler)
    // is supported.
    [[nodiscard]] std::size_t dispatch(Event event);

    // Return the number of subscribers for the given type.
    [[nodiscard]] std::size_t subscriber_count(const std::string& type) const;

    // Return the total number of subscriptions.
    [[nodiscard]] std::size_t total_subscribers() const noexcept;

    // Return metadata for a subscription, or std::nullopt if unknown.
    [[nodiscard]] std::optional<SubscriptionMetadata>
    subscription_metadata(SubscriptionId subscription_id) const;

    // Dispatch statistics.
    struct Statistics {
        std::uint64_t total_dispatched = 0;
        std::uint64_t total_delivered = 0;
        std::uint64_t total_filtered_out = 0;
        std::uint64_t total_handler_exceptions = 0;
        std::uint64_t active_nested_dispatches = 0;
    };

    [[nodiscard]] Statistics statistics() const noexcept;

private:
    struct Subscription {
        SubscriptionId id;
        std::string type;
        EventHandler handler;
        EventFilter filter;
        std::uint64_t dispatch_count = 0;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string,
                       std::vector<Subscription>>
        subscribers_;
    std::atomic<SubscriptionId> next_id_{1};

    // Dispatch statistics (updated under mutex_).
    std::uint64_t total_dispatched_ = 0;
    std::uint64_t total_delivered_ = 0;
    std::uint64_t total_filtered_out_ = 0;
    std::uint64_t total_handler_exceptions_ = 0;

    // Track active nested dispatches so we know whether we are inside a
    // dispatch call.
    std::atomic<std::size_t> active_nested_dispatches_{0};
};

}  // namespace nova::runtime
