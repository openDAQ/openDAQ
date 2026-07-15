#include <gtest/gtest.h>

#include <opendaq/scheduler_factory.h>
#include <opendaq/work_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/work_repetitive_ptr.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace daq;

class PeriodicWorkTest : public testing::Test
{
protected:
    SchedulerPtr scheduler = Scheduler(Logger(), 4);
};

TEST_F(PeriodicWorkTest, ExecutesPeriodically)
{
    std::atomic<int> counter{0};

    const auto work = WorkRepetitive(20, [&counter] { ++counter; });
    scheduler.scheduleWork(work);

    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    work.cancel();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    EXPECT_GE(counter.load(), 2);
}

TEST_F(PeriodicWorkTest, CancelAfterCallbackRunsAfterInFlightExecution)
{
    std::atomic<bool> afterCalled{false};
    std::atomic<bool> allowFinish{false};
    std::mutex mutex;
    std::condition_variable cv;

    const auto work = WorkRepetitive(5, [&] {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return allowFinish.load(); });
    });

    scheduler.scheduleWork(work);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    work->cancel(Work([&afterCalled] { afterCalled = true; }));

    EXPECT_FALSE(afterCalled.load());

    {
        std::lock_guard lock(mutex);
        allowFinish = true;
    }
    cv.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(afterCalled.load());
}

TEST_F(PeriodicWorkTest, ReschedulesWithCancelAfter)
{
    std::atomic<int> counter{0};
    auto work = WorkRepetitive(15, [&counter] { ++counter; });

    scheduler.scheduleWork(work);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    work->cancel(Work([&] {
        work = WorkRepetitive(15, [&counter] { ++counter; });
        scheduler.scheduleWork(work);
    }));

    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    work.cancel();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    EXPECT_GE(counter.load(), 2);
}

TEST_F(PeriodicWorkTest, UsesIntervalFromWorkRepetitive)
{
    std::atomic<int> counter{0};

    const auto work = WorkRepetitive(25, [&counter] { ++counter; });
    scheduler.scheduleWork(work);

    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    work.cancel();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    EXPECT_GE(counter.load(), 2);
}

TEST_F(PeriodicWorkTest, CancelSkipsFurtherExecutions)
{
    std::atomic<bool> executed{false};

    const auto work = WorkRepetitive(10, [&executed] { executed = true; });
    scheduler.scheduleWork(work);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    work.cancel();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    EXPECT_TRUE(executed.load());
    EXPECT_TRUE(work.isCanceled());
}
