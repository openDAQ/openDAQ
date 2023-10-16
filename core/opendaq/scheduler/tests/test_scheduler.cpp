#include "test_scheduler.h"
#include <gtest/gtest.h>

#include <thread>
#include <chrono>

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
