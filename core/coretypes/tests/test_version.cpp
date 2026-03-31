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

    ASSERT_EQ(major, OPENDAQ_CORETYPES_MAJOR_VERSION);
    ASSERT_EQ(minor, OPENDAQ_CORETYPES_MINOR_VERSION);
    ASSERT_EQ(revision, OPENDAQ_CORETYPES_PATCH_VERSION);
}
