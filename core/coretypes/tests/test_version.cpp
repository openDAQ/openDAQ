#include <gtest/gtest.h>
#include <coretypes/version.h>
#include <coretypes/coretypes_config.h>
#include <coretypes/exceptions.h>
#include <testutils/testutils.h>

using namespace daq;

using VersionTest = testing::Test;

TEST_F(VersionTest, CheckVersion)
{
    unsigned int major;
    unsigned int minor;
    unsigned int revision;

    ASSERT_THROW_MSG(
        daqCoreTypesGetVersion(&major, &minor, &revision),
        NotCompatibleVersionException,
        "does not support obsolete mechanism for checking core dependencies version"
    );
}
