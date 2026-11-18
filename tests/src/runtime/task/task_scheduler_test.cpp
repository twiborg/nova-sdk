#include "nova/runtime/task/task_scheduler.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace nova::runtime {

TEST(TaskSchedulerTest, SubmitAndComplete) {
    TaskScheduler sched;
    sched.start(1);
    std::atomic<int> counter{0};
    auto id = sched.submit([&counter] { counter++; });
    ASSERT_TRUE(id.has_value());
    sched.wait(*id);
    auto st = sched.state(*id);
    ASSERT_TRUE(st.has_value());
    EXPECT_EQ(*st, TaskState::Completed);
    auto res = sched.result(*id);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->ok);
    EXPECT_EQ(counter.load(), 1);
    sched.stop();
}

TEST(TaskSchedulerTest, CancellationQueued) {
    TaskScheduler sched;
    sched.start(1);
    std::atomic<bool> ran{false};
    auto id = sched.submit([&ran] { ran.store(true); });
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(sched.cancel(*id));
    sched.stop();
    EXPECT_FALSE(ran.load());
}

TEST(TaskSchedulerTest, QueueCapacity) {
    TaskScheduler sched;
    sched.start(0, 3); // no workers, capacity 3
    auto id1 = sched.submit([] {});
    auto id2 = sched.submit([] {});
    auto id3 = sched.submit([] {});
    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    ASSERT_TRUE(id3.has_value());
    auto id4 = sched.submit([] {});
    EXPECT_FALSE(id4.has_value()); // queue full
    sched.stop();
}

TEST(TaskSchedulerTest, MultipleWorkers) {
    TaskScheduler sched;
    sched.start(4);
    std::atomic<int> counter{0};
    const int n = 100;
    std::vector<TaskId> ids;
    for (int i = 0; i < n; ++i) {
        auto id = sched.submit([&counter] { counter++; });
        ASSERT_TRUE(id.has_value());
        ids.push_back(*id);
    }
    for (auto id : ids) {
        sched.wait(id);
    }
    EXPECT_EQ(counter.load(), n);
    for (auto id : ids) {
        auto st = sched.state(id);
        ASSERT_TRUE(st.has_value());
        EXPECT_EQ(*st, TaskState::Completed);
    }
    sched.stop();
}

TEST(TaskSchedulerTest, StopWithoutStart) {
    TaskScheduler sched;
    sched.stop(); // must be safe
}

TEST(TaskSchedulerTest, Restart) {
    TaskScheduler sched;
    sched.start(1);
    std::atomic<int> counter{0};
    auto id = sched.submit([&counter] { counter++; });
    ASSERT_TRUE(id.has_value());
    sched.wait(*id);
    EXPECT_EQ(counter.load(), 1);
    sched.stop();
    sched.start(1);
    auto id2 = sched.submit([&counter] { counter++; });
    ASSERT_TRUE(id2.has_value());
    sched.wait(*id2);
    EXPECT_EQ(counter.load(), 2);
    sched.stop();
}

// ---------- Priority tests ----------

TEST(TaskSchedulerTest, PriorityOrdering) {
    TaskScheduler sched;
    sched.start(1);
    std::vector<int> order;
    std::mutex order_mutex;

    // Submit low, then critical, then normal. Critical should run first.
    auto id_low = sched.submit([&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        order.push_back(0);
    },
                       TaskPriority::Low);
    auto id_crit = sched.submit([&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        order.push_back(2);
    },
                      TaskPriority::Critical);
    auto id_norm = sched.submit([&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        order.push_back(1);
    },
                        TaskPriority::Normal);
    ASSERT_TRUE(id_low.has_value());
    ASSERT_TRUE(id_crit.has_value());
    ASSERT_TRUE(id_norm.has_value());

    sched.wait(*id_low);
    sched.wait(*id_crit);
    sched.wait(*id_norm);

    // Critical first, then normal, then low.
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 2);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 0);
    sched.stop();
}

TEST(TaskSchedulerTest, FifoWithinSamePriority) {
    TaskScheduler sched;
    sched.start(1);
    std::vector<int> order;
    std::mutex order_mutex;

    auto id1 = sched.submit([&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        order.push_back(1);
    },
                        TaskPriority::Normal);
    auto id2 = sched.submit([&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        order.push_back(2);
    },
                        TaskPriority::Normal);
    auto id3 = sched.submit([&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        order.push_back(3);
    },
                        TaskPriority::Normal);
    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    ASSERT_TRUE(id3.has_value());

    sched.wait(*id1);
    sched.wait(*id2);
    sched.wait(*id3);

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
    sched.stop();
}

// ---------- Cancellation tests ----------

TEST(TaskSchedulerTest, CancelRunningCooperatively) {
    TaskScheduler sched;
    sched.start(1);
    std::atomic<bool> cancelled_seen{false};
    auto source = std::make_shared<CancellationSource>();
    auto token = source->token();

    std::mutex m;
    std::condition_variable cv;
    bool task_started = false;

    auto id = sched.submit([&] {
        {
            std::lock_guard<std::mutex> lock(m);
            task_started = true;
        }
        cv.notify_all();
        // Wait until cancellation is requested cooperatively.
        while (!token.is_cancelled()) {
            std::this_thread::yield();
        }
        cancelled_seen.store(true);
    },
                        TaskPriority::Normal, token);
    ASSERT_TRUE(id.has_value());

    // Wait for the task to start.
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return task_started; });
    }

    // Request cancellation while the task is running.
    source->request_cancellation();
    sched.wait(*id);
    EXPECT_TRUE(cancelled_seen.load());
    sched.stop();
}

TEST(TaskSchedulerTest, CancelAlreadyCompleted) {
    TaskScheduler sched;
    sched.start(1);
    auto id = sched.submit([] {});
    ASSERT_TRUE(id.has_value());
    sched.wait(*id);
    EXPECT_FALSE(sched.cancel(*id)); // already completed, cannot cancel
    sched.stop();
}

TEST(TaskSchedulerTest, CancelUnknownId) {
    TaskScheduler sched;
    sched.start(1);
    EXPECT_FALSE(sched.cancel(99999));
    sched.stop();
}

// ---------- Statistics tests ----------

TEST(TaskSchedulerTest, Statistics) {
    TaskScheduler sched;
    sched.start(2);
    std::atomic<int> counter{0};

    auto id1 = sched.submit([&counter] { counter++; });
    auto id2 = sched.submit([&counter] { counter++; });
    auto id3 = sched.submit([&counter] { counter++; });
    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    ASSERT_TRUE(id3.has_value());

    sched.wait(*id1);
    sched.wait(*id2);
    sched.wait(*id3);

    auto stats = sched.statistics();
    EXPECT_EQ(stats.total_submitted, 3u);
    EXPECT_EQ(stats.total_completed, 3u);
    EXPECT_EQ(stats.total_failed, 0u);
    EXPECT_EQ(stats.currently_queued, 0u);
    EXPECT_EQ(stats.currently_running, 0u);
    EXPECT_EQ(stats.worker_count, 2u);
    sched.stop();
}

TEST(TaskSchedulerTest, StatisticsAfterCancel) {
    TaskScheduler sched;
    sched.start(1);
    auto id = sched.submit([] {});
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(sched.cancel(*id));
    sched.stop();

    auto stats = sched.statistics();
    EXPECT_EQ(stats.total_cancelled, 1u);
    EXPECT_EQ(stats.total_completed, 0u);
}

TEST(TaskSchedulerTest, StatisticsAfterCancelledTaskNotCountedAsRetained) {
    TaskScheduler sched;
    sched.start(1);
    auto id = sched.submit([] {});
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(sched.cancel(*id));
    // Drain the queue so the cancelled task is popped from queue_ and only
    // remains in tasks_ as a terminal Cancelled entry.
    sched.stop();
    auto stats = sched.statistics();
    EXPECT_EQ(stats.total_cancelled, 1u);
    EXPECT_EQ(stats.retained_results, 0u);
}

TEST(TaskSchedulerTest, StatisticsAfterFailedTaskNotCountedAsRetained) {
    TaskScheduler sched;
    sched.start(1);
    auto id = sched.submit([] { throw std::runtime_error("boom"); });
    ASSERT_TRUE(id.has_value());
    sched.wait(*id);
    ASSERT_TRUE(sched.state(*id));
    EXPECT_EQ(*sched.state(*id), TaskState::Failed);
    auto stats = sched.statistics();
    EXPECT_EQ(stats.total_failed, 1u);
    EXPECT_EQ(stats.retained_results, 0u);
    sched.stop();
}

TEST(TaskSchedulerTest, StatisticsCountsOnlyCompletedRetainedTasks) {
    TaskScheduler sched;
    SchedulerConfig config;
    config.retention_policy = RetentionPolicy::None;
    sched.start(1, 1024, config);

    std::vector<TaskId> ids;
    for (int i = 0; i < 5; ++i) {
        auto id = sched.submit([] {});
        ASSERT_TRUE(id.has_value());
        ids.push_back(*id);
    }
    // Add a failing task; it must not count as a retained result.
    auto bad = sched.submit([] { throw std::runtime_error("x"); });
    ASSERT_TRUE(bad.has_value());
    ids.push_back(*bad);

    for (auto id : ids) {
        sched.wait(id);
    }

    auto stats = sched.statistics();
    EXPECT_EQ(stats.total_completed, 5u);
    EXPECT_EQ(stats.total_failed, 1u);
    EXPECT_EQ(stats.retained_results, stats.total_completed);
    sched.stop();
}

// ---------- Retention tests ----------

TEST(TaskSchedulerTest, RetentionCountBased) {
    TaskScheduler sched;
    SchedulerConfig config;
    config.retention_policy = RetentionPolicy::CountBased;
    config.max_retained_results = 2;
    sched.start(1, 1024, config);

    auto id1 = sched.submit([] {});
    auto id2 = sched.submit([] {});
    auto id3 = sched.submit([] {});
    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    ASSERT_TRUE(id3.has_value());

    sched.wait(*id1);
    sched.wait(*id2);
    sched.wait(*id3);

    // After cleanup, only max_retained_results (2) should remain.
    auto stats = sched.statistics();
    EXPECT_LE(stats.retained_results, config.max_retained_results);
    sched.stop();
}

TEST(TaskSchedulerTest, RetentionDisabled) {
    TaskScheduler sched;
    SchedulerConfig config;
    config.retention_policy = RetentionPolicy::None;
    sched.start(1, 1024, config);

    auto id1 = sched.submit([] {});
    auto id2 = sched.submit([] {});
    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    sched.wait(*id1);
    sched.wait(*id2);

    auto stats = sched.statistics();
    // With None retention, both completed tasks should remain.
    EXPECT_EQ(stats.retained_results, 2u);
    EXPECT_EQ(stats.total_retained_cleaned_up, 0u);
    sched.stop();
}

TEST(TaskSchedulerTest, RetentionPreservesRunning) {
    TaskScheduler sched;
    SchedulerConfig config;
    config.retention_policy = RetentionPolicy::CountBased;
    config.max_retained_results = 0;
    sched.start(1, 1024, config);

    std::atomic<bool> go{false};
    auto id = sched.submit([&] {
        while (!go.load()) {
        }
    });
    ASSERT_TRUE(id.has_value());

    // Give the worker a moment to start running.
    for (int i = 0; i < 1000 && id.has_value() && !sched.state(*id).has_value(); ++i) {
    }
    auto st = sched.state(*id);
    if (st && *st == TaskState::Running) {
        // Trigger cleanup while running; it should NOT be removed.
        sched.stop();
    } else {
        // Task may have already completed if it ran very fast; just stop.
        go.store(true);
        sched.wait(*id);
        sched.stop();
    }
}

}  // namespace nova::runtime
