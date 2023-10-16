#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using ObjCountTest = testing::Test;

#ifndef NDEBUG

TEST_F(ObjCountTest, CountObjects)
{
    size_t val1 = daqGetTrackedObjectCount();
    size_t val2 = daqGetTrackedObjectCount();
    ASSERT_EQ(val1, val2);

    auto obj = BaseObject();
    size_t val3 = daqGetTrackedObjectCount();
    ASSERT_EQ(val1 + 1, val3);
}

#endif
