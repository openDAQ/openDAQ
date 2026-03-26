#include <gtest/gtest.h>
#include <coretypes/version.h>
#include <coretypes/coretypes_config.h>

using namespace daq;

using VersionTest = testing::Test;

TEST_F(VersionTest, CheckVersion)
{
    unsigned int major;
    unsigned int minor;
    unsigned int revision;
    daqCoreTypesGetVersion(&major, &minor, &revision);

    ASSERT_EQ(major, 0u);
    ASSERT_EQ(minor, 0u);
    ASSERT_EQ(revision, 0u);
}
