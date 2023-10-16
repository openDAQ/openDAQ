#include <gtest/gtest.h>
#include <opendaq/task_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/logger_factory.h>

using namespace daq;

static ErrCode testTask(IBaseObject* /*obj*/)
{
    return OPENDAQ_SUCCESS;
}

using TaskTest = testing::Test;

using namespace daq;

TEST_F(TaskTest, RootTask)
{
    ASSERT_NO_THROW(TaskGraph(testTask, "Root"));
}

TEST_F(TaskTest, NameRootTask)
{
    auto sub = TaskGraph(testTask, "Root");
    sub.setName("test");
}

TEST_F(TaskTest, SubTask)
{
    ASSERT_NO_THROW(Task(testTask, "Sub"));
}

TEST_F(TaskTest, NameSubTask)
{
    auto sub = Task(testTask, "Sub");
    sub.setName("test");
}

TEST_F(TaskTest, RootTaskThenSelf)
{
    auto root = TaskGraph(testTask, "Root");
    ASSERT_EQ(root->then(root.getObject()), OPENDAQ_ERR_NOT_SUPPORTED);
}

TEST_F(TaskTest, RootTaskThenRootTask)
{
    auto root1 = TaskGraph(testTask, "1");
    auto root2 = TaskGraph(testTask, "2");

    ASSERT_EQ(root1->then(root2.getObject()), OPENDAQ_ERR_NOT_SUPPORTED);
}

TEST_F(TaskTest, CircularReference)
{
    std::atomic<bool> executed(false);
    auto root = TaskGraph(testTask, "root");

    auto sub = Task([&executed] { executed = true; });
    root.then(sub);
    sub.then(sub);

    auto scheduler = Scheduler(Logger());
    scheduler.scheduleGraph(root).wait();

    ASSERT_FALSE(executed);
}

TEST_F(TaskTest, ScheduleGraphMasksExceptions)
{
    auto root = TaskGraph([] {
        throw std::runtime_error("MockException");
    }, "root");

    auto scheduler = Scheduler(Logger());

    auto aw = scheduler.scheduleGraph(root);
    aw.wait();

    ASSERT_NO_THROW(auto result = aw.getResult());
}
