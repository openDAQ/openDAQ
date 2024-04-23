#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using RegressionTestDevice = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(RegressionTestDevice, Mock)
{
    ASSERT_EQ(6, 6);
    ASSERT_NE(6, 7);
}

END_NAMESPACE_OPENDAQ
