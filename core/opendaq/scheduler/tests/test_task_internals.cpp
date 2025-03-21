#include <opendaq/scheduler_factory.h>
#include <opendaq/task_factory.h>
#include <opendaq/task_internal.h>
#include <gtest/gtest.h>
#include <testutils/ut_logging.h>

#include <chrono>
#include <opendaq/task_flow.h>

using TaskInternalsTest = testing::Test;

using namespace daq;
using namespace std::chrono_literals;

TEST_F(TaskInternalsTest, TaskFlowFutureDestructorDoesNotBlock)
{
    using namespace std::chrono_literals;

    bool finished{false};
    tf::Executor e;
    {
        auto future = e.async([&finished] {
            std::this_thread::sleep_for(std::chrono::seconds(2));

            finished = true;
            return 3;
        });
    }
    std::cout << "After" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
    EXPECT_TRUE(finished);
}
