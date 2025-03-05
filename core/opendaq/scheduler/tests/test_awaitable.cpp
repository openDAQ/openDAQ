#include <testutils/testutils.h>
#include <opendaq/awaitable_ptr.h>
#include <opendaq/scheduler_exceptions.h>
#include <opendaq/work_factory.h>

#include "test_scheduler.h"

#include <thread>
#include <atomic>

using namespace daq;

class AwaitableTest : public SchedulerTest
{
public:
    AwaitableTest() : SchedulerTest(1)
    {
    }
};

TEST_F(AwaitableTest, FunctionWait)
{
    auto returnValue = 1;

    auto awaitable = scheduler.scheduleFunction([returnValue]()
    {
        return returnValue;
    });

    awaitable.wait();
    Int result = awaitable.getResult();

    ASSERT_EQ(result, returnValue);
}

TEST_F(AwaitableTest, GetResultBlocks)
{
    auto returnValue = 1;
    std::atomic<bool> executed(false);

    auto awaitable = scheduler.scheduleFunction([returnValue, &executed]()
    {
        using namespace std::literals;

        std::this_thread::sleep_for(2s);
        executed = true;
        return returnValue;
    });

    ASSERT_FALSE(awaitable.hasCompleted());
    ASSERT_FALSE(executed);

    Int result = awaitable.getResult();

    ASSERT_TRUE(awaitable.hasCompleted());
    ASSERT_TRUE(executed);

    ASSERT_EQ(result, returnValue);
}

TEST_F(AwaitableTest, CancelNotYetExecuted)
{
    auto blocker = scheduler.scheduleFunction([]() {
        using namespace std::literals;

        std::this_thread::sleep_for(2s);
        return 1;
    });

    std::atomic<bool> executed(false);
    auto awaitable = scheduler.scheduleFunction([&executed](bool canceled)
    {
        executed = true;
        return 2;
    });

    ASSERT_TRUE(awaitable.cancel());
    awaitable.wait();

    ASSERT_FALSE(executed);
    blocker.wait();
}

TEST_F(AwaitableTest, FunctionThrows)
{
    auto scheduler = Scheduler(daq::Logger());

    auto awaitable = scheduler.scheduleFunction([]() -> Int {
        throw SchedulerUnknownException("MockException");
    });

    ASSERT_NO_THROW(awaitable.wait());
    ASSERT_THROW_MSG(awaitable.getResult(), SchedulerUnknownException, "MockException")
}

TEST_F(AwaitableTest, Work)
{
    std::mutex mut;
    std::condition_variable cv;

    auto workExecuted = false;
    scheduler.scheduleWork(Work([&workExecuted, &mut, &cv]()
        {
            std::unique_lock lock(mut);
            workExecuted = true;
            cv.notify_one();
        }));

    std::unique_lock lock(mut);
    while (!workExecuted)
        cv.wait(lock);
}
