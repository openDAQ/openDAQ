#include <gtest/gtest.h>
#include <coreobjects/version.h>
#include <coreobjects/coreobjects_config.h>

using namespace daq;

using VersionTest = testing::Test;

TEST_F(VersionTest, CheckVersion)
{
    unsigned int major;
    unsigned int minor;
    unsigned int revision;
    daqCoreObjectsGetVersion(&major, &minor, &revision);

    ASSERT_EQ(major, OPENDAQ_COREOBJECTS_MAJOR_VERSION);
    ASSERT_EQ(minor, OPENDAQ_COREOBJECTS_MINOR_VERSION);
    ASSERT_EQ(revision, OPENDAQ_COREOBJECTS_PATCH_VERSION);
}
