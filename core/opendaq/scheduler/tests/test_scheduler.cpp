#include "test_scheduler.h"
#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <future>
#include <opendaq/work_factory.h>

#include <opendaq/logger_factory.h>

class SchedulerTestCommon : public testing::Test
{
protected:
    void TearDown() override
    {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(70ms);
    }
};

using namespace daq;

TEST_F(SchedulerTestCommon, Create)
{
    ASSERT_NO_THROW(Scheduler(Logger()));
}

TEST_F(SchedulerTestCommon, DefaultIsMultiThreaded)
{
    auto scheduler = Scheduler(Logger());
    ASSERT_TRUE(scheduler.isMultiThreaded());
}

TEST_F(SchedulerTestCommon, SingleThreaded)
{
    auto scheduler = Scheduler(Logger(), 1);
    ASSERT_FALSE(scheduler.isMultiThreaded());
}

TEST_F(SchedulerTestCommon, GraphExceptionsMaskedByDefault)
{
    auto root = TaskGraph([]() {
        throw std::runtime_error("Will be ignored");
    }, "root");

    auto scheduler = Scheduler(Logger());
    auto aw = scheduler.scheduleGraph(root);

    ASSERT_NO_THROW(aw.getResult());
}

TEST_F(SchedulerTestCommon, ImplicitTask)
{
    std::atomic<Int> executed{0};

    auto root = TaskGraph("root");
    root.then([&executed]()
    {
        ++executed;
    }, "A")
    .then([&executed]()
    {
        ++executed;
    }, "B");

    auto scheduler = Scheduler(Logger());
    auto aw = scheduler.scheduleGraph(root);

    ASSERT_NO_THROW(aw.getResult());
    ASSERT_EQ(executed, 2);
}

TEST_F(SchedulerTestCommon, ExecutesOneTimeWork)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);
    bool called = false;

    auto work = WorkRepetitive([&called] 
    {
        called = true;
    });
    scheduler.scheduleWorkOnMainLoop(work);

    scheduler.runMainLoopIteration();
    ASSERT_TRUE(called);
}

TEST_F(SchedulerTestCommon, ScheduleWorkOnMainLoopThrowsIfMainLoopNotSet)
{
    auto scheduler = Scheduler(Logger(), 1);
    auto work = WorkRepetitive([]() -> void {});
    ASSERT_THROW(scheduler.scheduleWorkOnMainLoop(work), NotSupportedException);
}

TEST_F(SchedulerTestCommon, RunMainLoopIterationThrowsIfMainLoopNotSet)
{
    auto scheduler = Scheduler(Logger(), 1);
    ASSERT_THROW(scheduler.runMainLoopIteration(), NotSupportedException);
}

TEST_F(SchedulerTestCommon, RunMainLoopThrowsIfMainLoopNotSet)
{
    auto scheduler = Scheduler(Logger(), 1);
    ASSERT_THROW(scheduler.runMainLoop(), NotSupportedException);
}

TEST_F(SchedulerTestCommon, RunMainLoopTwice)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);

    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    auto thread = std::thread([&]() -> void
    {
        promise.set_value();
        scheduler.runMainLoop();
    });

    future.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_THROW(scheduler.runMainLoop(), daq::InvalidOperationException);
    scheduler.stopMainLoop();
    thread.join();
}

TEST_F(SchedulerTestCommon, ExecutesRepetitiveWork)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);
    int counter = 0;

    auto work = WorkRepetitive([&counter]() -> bool
    {
        return ++counter < 3;
    });
    scheduler.scheduleWorkOnMainLoop(work);

    for (int i = 0; i < 5; ++i)
        scheduler.runMainLoopIteration();

    ASSERT_EQ(counter, 3);
}

TEST_F(SchedulerTestCommon, RepetitiveVoidWorkExecutesOnce)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);
    int counter = 0;

    auto work = WorkRepetitive([&counter]() -> void 
    {
        ++counter;
    });

    scheduler.scheduleWorkOnMainLoop(work);
    scheduler.runMainLoopIteration();
    scheduler.runMainLoopIteration();

    ASSERT_EQ(counter, 1);
}

TEST_F(SchedulerTestCommon, StartsAndStops)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);

    std::thread loopThread([&]()
    {
        scheduler.runMainLoop();
    });

    bool called = false;
    auto work = WorkRepetitive([&called]() -> void 
    {
        called = true;
    });
    scheduler.scheduleWorkOnMainLoop(work);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    scheduler.stopMainLoop();
    loopThread.join();

    ASSERT_TRUE(called);
}

TEST_F(SchedulerTestCommon, StartsAndStopsFromWork)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);

    bool called = false;
    auto work = WorkRepetitive([&]() -> void 
    {
        called = true;
        scheduler.stopMainLoop();
    });
    scheduler.scheduleWorkOnMainLoop(work);
    scheduler.runMainLoop();
    ASSERT_TRUE(called);
}

TEST_F(SchedulerTestCommon, ExecutesOneTimeWorkWithTimeLoop)
{
    auto scheduler = SchedulerWithMainLoop(Logger(), 1);

    bool called = false;
    auto work = WorkRepetitive([&]() -> void 
    {
        called = true;
        scheduler.stopMainLoop();
    });
    scheduler.scheduleWorkOnMainLoop(work);

    SizeT loopTime = 10; // milliseconds
    auto begin = std::chrono::steady_clock::now();
    scheduler.runMainLoop(loopTime);
    auto end = std::chrono::steady_clock::now();
    ASSERT_TRUE(end - begin >= std::chrono::milliseconds(loopTime));
}