#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include "test_scheduler.h"
#include <gmock/gmock.h>
#include <opendaq/scheduler_ptr.h>

using SchedulerTestMt = SchedulerTest;

using namespace testing;
using namespace daq;

TEST_F(SchedulerTestMt, Create)
{
    ASSERT_NO_THROW(Scheduler(daq::Logger()));
}

TEST_F(SchedulerTestMt, Graph1)
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
        "Root");

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
                    ElementsAre('a', 'b', 'd', 'c', 'e'),
                    ElementsAre('a', 'c', 'b', 'd', 'e'),
                    ElementsAre('b', 'a', 'd', 'c', 'e'),
                    ElementsAre('b', 'a', 'c', 'd', 'e')
                ));
}

TEST_F(SchedulerTestMt, Graph2)
{
    /*
     *                     f
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

    ASSERT_THAT(order,
                AnyOf(ElementsAre('a', 'f', 'b', 'c', 'd', 'e'),
                      ElementsAre('a', 'f', 'b', 'c', 'd', 'e'),
                      ElementsAre('a', 'f', 'b', 'd', 'c', 'e'),
                      ElementsAre('a', 'f', 'c', 'b', 'd', 'e'),
                      ElementsAre('f', 'a', 'b', 'c', 'd', 'e'),
                      ElementsAre('f', 'a', 'c', 'b', 'd', 'e'),
                      ElementsAre('a', 'c', 'f', 'b', 'd', 'e'),
                      ElementsAre('f', 'a', 'b', 'c', 'd', 'e'),
                      ElementsAre('f', 'b', 'a', 'd', 'c', 'e'),
                      ElementsAre('f', 'b', 'a', 'c', 'd', 'e'),
                      ElementsAre('f', 'a', 'b', 'd', 'c', 'e'),
                      ElementsAre('a', 'c', 'f', 'b', 'd', 'e')
                ));
}
