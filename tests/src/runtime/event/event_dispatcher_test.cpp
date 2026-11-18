#include "nova/runtime/event/event_dispatcher.h"

#include <any>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

nova::runtime::Event make_event(nova::runtime::EventId id,
                                const std::string& type) {
    return nova::runtime::Event{id, type, std::any(),
                                std::chrono::system_clock::now()};
}

}  // namespace

TEST(EventDispatcherTest, SubscribeAndDispatch) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> counter{0};
    auto id = disp.subscribe("test.event",
                             [&counter](const auto&) { counter++; });
    (void)id;
    auto ev = make_event(1, "test.event");
    EXPECT_EQ(disp.dispatch(ev), 1u);
    EXPECT_EQ(counter.load(), 1);
}

TEST(EventDispatcherTest, MultipleSubscribers) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> a{0}, b{0};
    auto id_a = disp.subscribe("evt", [&a](const auto&) { a++; });
    auto id_b = disp.subscribe("evt", [&b](const auto&) { b++; });
    (void)id_a;
    (void)id_b;
    auto ev = make_event(1, "evt");
    EXPECT_EQ(disp.dispatch(ev), 2u);
    EXPECT_EQ(a.load(), 1);
    EXPECT_EQ(b.load(), 1);
}

TEST(EventDispatcherTest, Unsubscription) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> counter{0};
    auto id = disp.subscribe("evt", [&counter](const auto&) { counter++; });
    EXPECT_TRUE(disp.unsubscribe(id));
    auto ev = make_event(1, "evt");
    EXPECT_EQ(disp.dispatch(ev), 0u);
    EXPECT_EQ(counter.load(), 0);
    EXPECT_FALSE(disp.unsubscribe(id)); // already removed
}

TEST(EventDispatcherTest, UnknownType) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> counter{0};
    auto id = disp.subscribe("evt", [&counter](const auto&) { counter++; });
    (void)id;
    auto ev = make_event(1, "other");
    EXPECT_EQ(disp.dispatch(ev), 0u);
    EXPECT_EQ(counter.load(), 0);
}

TEST(EventDispatcherTest, CallbackExceptionIsolation) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> second{0};
    auto id1 = disp.subscribe("evt", [](const auto&) {
        throw std::runtime_error("boom");
    });
    auto id2 = disp.subscribe("evt", [&second](const auto&) { second++; });
    (void)id1;
    (void)id2;
    auto ev = make_event(1, "evt");
    EXPECT_EQ(disp.dispatch(ev), 1u); // only the second handler runs
    EXPECT_EQ(second.load(), 1);
}

TEST(EventDispatcherTest, ConcurrentSubscribeAndDispatch) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> counter{0};
    auto id = disp.subscribe("evt", [&counter](const auto&) { counter++; });
    (void)id;
    auto ev = make_event(1, "evt");
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&disp, &ev] {
            for (int j = 0; j < 100; ++j) {
                (void)disp.dispatch(ev);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(counter.load(), 1000);
}

// ---------- Event filtering tests ----------

TEST(EventDispatcherTest, EventFiltering) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> counter{0};
    auto id = disp.subscribe(
        "evt", [&counter](const auto&) { counter++; },
        [](const nova::runtime::Event& e) { return e.id % 2 == 0; });
    (void)id;
    auto ev1 = make_event(1, "evt");
    auto ev2 = make_event(2, "evt");
    EXPECT_EQ(disp.dispatch(ev1), 0u); // filtered out
    EXPECT_EQ(disp.dispatch(ev2), 1u); // delivered
    EXPECT_EQ(counter.load(), 1);
}

// ---------- Subscription metadata tests ----------

TEST(EventDispatcherTest, SubscriptionMetadata) {
    nova::runtime::EventDispatcher disp;
    auto id = disp.subscribe("metadata.type", [](const auto&) {});
    auto meta = disp.subscription_metadata(id);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->id, id);
    EXPECT_EQ(meta->type, "metadata.type");
    EXPECT_EQ(meta->dispatch_count, 0u);

    auto ev = make_event(1, "metadata.type");
    (void)disp.dispatch(ev);
    meta = disp.subscription_metadata(id);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->dispatch_count, 1u);
}

// ---------- Unsubscribe during dispatch tests ----------

TEST(EventDispatcherTest, UnsubscribeDuringDispatch) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> first{0}, second{0};
    auto id1 = disp.subscribe("evt", [&first](const auto&) { first++; });
    auto id2 = disp.subscribe("evt", [&disp, id1, &second](const auto&) {
        second++;
        (void)disp.unsubscribe(id1); // unsubscribe the other handler during dispatch
    });
    (void)id1;
    (void)id2;

    // First dispatch: both handlers are present at start, so both run.
    auto ev = make_event(1, "evt");
    EXPECT_EQ(disp.dispatch(ev), 2u);
    EXPECT_EQ(first.load(), 1);
    EXPECT_EQ(second.load(), 1);

    // Second dispatch: id1 has been unsubscribed, only id2 runs.
    EXPECT_EQ(disp.dispatch(ev), 1u);
    EXPECT_EQ(first.load(), 1); // unchanged
    EXPECT_EQ(second.load(), 2);
}

// ---------- Nested dispatch tests ----------

TEST(EventDispatcherTest, NestedDispatch) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> outer{0}, inner{0};
    auto id_outer = disp.subscribe(
        "outer", [&outer, &disp](const auto&) {
            outer++;
            // Dispatch a nested event from within a handler.
            auto inner_ev = make_event(2, "inner");
            (void)disp.dispatch(inner_ev);
        });
    auto id_inner = disp.subscribe("inner", [&inner](const auto&) { inner++; });
    (void)id_outer;
    (void)id_inner;

    auto ev = make_event(1, "outer");
    EXPECT_EQ(disp.dispatch(ev), 1u);
    EXPECT_EQ(outer.load(), 1);
    EXPECT_EQ(inner.load(), 1);
}

// ---------- Dispatch statistics tests ----------

TEST(EventDispatcherTest, DispatchStatistics) {
    nova::runtime::EventDispatcher disp;
    std::atomic<int> counter{0};
    auto id = disp.subscribe("evt", [&counter](const auto&) { counter++; });
    auto id_other = disp.subscribe("other", [](const auto&) {});
    (void)id;
    (void)id_other;

    auto ev1 = make_event(1, "evt");
    auto ev2 = make_event(2, "other");
    auto ev3 = make_event(3, "evt");

    (void)disp.dispatch(ev1);
    (void)disp.dispatch(ev2);
    (void)disp.dispatch(ev3);

    auto stats = disp.statistics();
    EXPECT_EQ(stats.total_dispatched, 3u);
    EXPECT_EQ(stats.total_delivered, 3u);
    EXPECT_EQ(stats.total_filtered_out, 0u);
    EXPECT_EQ(stats.active_nested_dispatches, 0u);
}

TEST(EventDispatcherTest, DispatchStatisticsFilterOut) {
    nova::runtime::EventDispatcher disp;
    auto id = disp.subscribe(
        "evt", [](const auto&) {},
        [](const nova::runtime::Event& e) { return e.id == 1; });
    (void)id;
    auto ev1 = make_event(1, "evt");
    auto ev2 = make_event(2, "evt");
    (void)disp.dispatch(ev1); // delivered
    (void)disp.dispatch(ev2); // filtered out

    auto stats = disp.statistics();
    EXPECT_EQ(stats.total_dispatched, 2u);
    EXPECT_EQ(stats.total_delivered, 1u);
    EXPECT_EQ(stats.total_filtered_out, 1u);
}
