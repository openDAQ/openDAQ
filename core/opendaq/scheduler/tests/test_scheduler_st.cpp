#include <opendaq/scheduler_factory.h>
#include <opendaq/task_factory.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "test_scheduler.h"

class SchedulerTestSt : public SchedulerTest
{
public:
    SchedulerTestSt() : SchedulerTest(1)
    {
    }
};

using namespace testing;
using namespace daq;

TEST_F(SchedulerTestSt, Create)
{
    ASSERT_NO_THROW(Scheduler(daq::Logger(), 1));
}

TEST_F(SchedulerTestSt, OnlyOutputs)
{
    /*
     *               root
     *          _______|______
     *         a-----         b
     *         |     \        |
     *         c      ---->   d
     *         |______        |
     *                \--->   e
     */

    auto start = TaskGraph("Root");

    start.then(a);
    start.then(b);

    a.then(c);
    a.then(d);

    b.then(d);

    c.then(e);

    d.then(e);

    auto awaitable = scheduler.scheduleGraph(start);
    ASSERT_NO_THROW(awaitable.wait());

    ASSERT_THAT(order,
        AnyOf(
            ElementsAre('a', 'b', 'c', 'd', 'e'),
            ElementsAre('b', 'a', 'd', 'c', 'e')
        )
    );
}

TEST_F(SchedulerTestSt, Graph1)
{
    /*
     *                root
     *          _______|______
     *         a-----         b
     *         |     \        |
     *         c      ---->   d
     *         |______        |
     *                \--->   e
     */

    auto start = TaskGraph(
        [](IBaseObject*) -> ErrCode {
            LOG("Start task")
    
            return OPENDAQ_SUCCESS;
        },
        "Root"
    );

    start.then(a);
    start.then(b);
    
    a.then(c);
    a.then(d);
    
    b.then(d);
    
    c.then(e);
    
    d.then(e);

    auto awaitable = scheduler.scheduleGraph(start);
    ASSERT_NO_THROW(awaitable.wait());

    ASSERT_THAT(order, AnyOf(
        ElementsAre('a', 'b', 'c', 'd', 'e'),
        ElementsAre('b', 'a', 'd', 'c', 'e')
    ));
}

TEST_F(SchedulerTestSt, Graph2)
{
    /*
     *                     f
     *                     ^
     *                     |
     *                     v
     *        (a)----     (b)
     *         |     \     |
     *         v      \    |
     *         c       --->d
     *         |______     |
     *                \--->e
     */

    auto root = TaskGraph("Root");
    root.then(a);
    root.then(b);

    a.then(c);
    a.then(d);

    b.then(d);
    f.then(b);

    c.then(e);

    d.then(e);

    auto awaitable = scheduler.scheduleGraph(root);

    ASSERT_NO_THROW(awaitable.wait());

    ASSERT_THAT(
        order,
        AnyOf(
            ElementsAre('a', 'f', 'b', 'c', 'd', 'e'),
            ElementsAre('a', 'f', 'b', 'c', 'd', 'e'),
            ElementsAre('a', 'f', 'c', 'b', 'd', 'e'),
            ElementsAre('f', 'a', 'b', 'c', 'd', 'e'),
            ElementsAre('f', 'a', 'c', 'b', 'd', 'e'),
            ElementsAre('a', 'c', 'f', 'b', 'd', 'e')
        )
    );
}
