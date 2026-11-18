#include "nova/runtime/event/event_dispatcher.h"

#include <utility>

namespace nova::runtime {

EventDispatcher::EventDispatcher() = default;

EventDispatcher::~EventDispatcher() = default;

SubscriptionId EventDispatcher::subscribe(std::string type,
                                          EventHandler handler,
                                          EventFilter filter) {
    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionId id = next_id_.fetch_add(1, std::memory_order_relaxed);
    Subscription sub;
    sub.id = id;
    sub.type = type;
    sub.handler = std::move(handler);
    sub.filter = std::move(filter);
    sub.dispatch_count = 0;
    subscribers_[type].push_back(std::move(sub));
    return id;
}

bool EventDispatcher::unsubscribe(SubscriptionId subscription_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : subscribers_) {
        auto& vec = pair.second;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it->id == subscription_id) {
                vec.erase(it);
                if (vec.empty()) {
                    subscribers_.erase(pair.first);
                }
                return true;
            }
        }
    }
    return false;
}

std::size_t EventDispatcher::dispatch(Event event) {
    std::vector<Subscription> handlers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++total_dispatched_;

        auto it = subscribers_.find(event.type);
        if (it == subscribers_.end()) {
            ++total_filtered_out_;
            return 0;
        }

        // Copy matching handlers (those whose filter passes).
        for (auto& sub : it->second) {
            if (!sub.filter || sub.filter(event)) {
                handlers.push_back(sub);
            } else {
                ++total_filtered_out_;
            }
        }
        ++active_nested_dispatches_;
    }

    std::size_t invoked = 0;
    for (auto& sub : handlers) {
        try {
            sub.handler(event);
            ++invoked;
            ++total_delivered_;
            // Update dispatch count under lock.
            {
                std::lock_guard<std::mutex> lock(mutex_);
                // Find and increment the original subscription's count.
                for (auto& pair : subscribers_) {
                    for (auto& s : pair.second) {
                        if (s.id == sub.id) {
                            ++s.dispatch_count;
                            break;
                        }
                    }
                }
            }
        } catch (...) {
            ++total_handler_exceptions_;
            // Exception isolation: a throwing handler does not prevent the
            // remaining handlers from running.
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        --active_nested_dispatches_;
    }
    return invoked;
}

std::size_t EventDispatcher::subscriber_count(const std::string& type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscribers_.find(type);
    return it == subscribers_.end() ? 0 : it->second.size();
}

std::size_t EventDispatcher::total_subscribers() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t total = 0;
    for (const auto& pair : subscribers_) {
        total += pair.second.size();
    }
    return total;
}

std::optional<SubscriptionMetadata>
EventDispatcher::subscription_metadata(SubscriptionId subscription_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : subscribers_) {
        for (const auto& sub : pair.second) {
            if (sub.id == subscription_id) {
                SubscriptionMetadata meta;
                meta.id = sub.id;
                meta.type = sub.type;
                meta.dispatch_count = sub.dispatch_count;
                return meta;
            }
        }
    }
    return std::nullopt;
}

EventDispatcher::Statistics EventDispatcher::statistics() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    Statistics stats;
    stats.total_dispatched = total_dispatched_;
    stats.total_delivered = total_delivered_;
    stats.total_filtered_out = total_filtered_out_;
    stats.total_handler_exceptions = total_handler_exceptions_;
    stats.active_nested_dispatches =
        active_nested_dispatches_.load(std::memory_order_acquire);
    return stats;
}

}  // namespace nova::runtime
