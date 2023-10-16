#include <opendaq/device_info_factory.h>
#include <gtest/gtest.h>

using DeviceFactoriesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DeviceFactoriesTest, TestDeviceInfoFactory)
{
    ASSERT_NO_THROW(DeviceInfo(""));
}

END_NAMESPACE_OPENDAQ
