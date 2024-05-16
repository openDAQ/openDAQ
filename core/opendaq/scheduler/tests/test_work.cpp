#include <gtest/gtest.h>
#include <opendaq/work_factory.h>

using namespace daq;

using WorkTest = testing::Test;

using namespace daq;

TEST_F(WorkTest, Execute)
{
    auto executed = false;
    const auto work = Work([&executed] { executed = true;  });
    work.execute();
    ASSERT_TRUE(executed);
}

TEST_F(WorkTest, QueryInterface)
{
    const auto work = Work([] {});

    const auto obj1 = work.asPtr<IBaseObject>();
    const auto work1 = obj1.asPtr<IWork>();

    const auto obj2 = work.asPtr<IBaseObject>(true);
    const auto work2 = obj2.asPtr<IWork>(true);
}
