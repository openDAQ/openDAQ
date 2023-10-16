#include <opendaq/scheduler_factory.h>
#include <opendaq/scheduler_impl.h>
#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <opendaq/logger_factory.h>

class SchedulerInternalsTest : public testing::Test
{
protected:
    void TearDown() override
    {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(100ms);
    }
};

using namespace testing;
using namespace daq;

TEST_F(SchedulerInternalsTest, DefaultWorkerThreads)
{
    auto scheduler = Scheduler(daq::Logger());
    auto impl = dynamic_cast<SchedulerImpl*>(scheduler.getObject());

    ASSERT_NE(impl, nullptr);
    ASSERT_EQ(impl->getWorkerCount(), std::thread::hardware_concurrency());
}
