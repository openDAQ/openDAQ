#include <testutils/bb_memcheck_listener.h>
#include <coretypes/coretypes.h>

using namespace daq;

using CylceRefTest = testing::Test;

TEST_F(CylceRefTest, Leak)
{
    MemCheckListener::expectMemoryLeak = true;

    auto list1 = List<IBaseObject>();
    auto list2 = List<IBaseObject>();
    list1.pushBack(list2);
    list2.pushBack(list1);
}

TEST_F(CylceRefTest, ManualBreak)
{
    auto list1 = List<IBaseObject>();
    auto list2 = List<IBaseObject>();
    list1.pushBack(list2);
    list2.pushBack(list1);
    list1.dispose();
}
