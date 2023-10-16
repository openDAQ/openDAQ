#include <gtest/gtest.h>
#include <coretypes/listptr.h>
#include <opendaq/module_check_dependencies.h>

using namespace daq;

using CheckDependenciesTest = testing::Test;

TEST_F(CheckDependenciesTest, TestCompiledMinorGreater)
{
    LibraryVersion compiledVersion
    {
        1,
        2,
        0,
    };

    auto linkedVersion = [](unsigned int* major, unsigned int* minor, unsigned int* revision)
    {
        *major = 1;
        *minor = 1;
        *revision = 0;
    };

    StringPtr errMsg;
    bool compatible = isCompatibleVersion("CoreTypes", linkedVersion, compiledVersion, &errMsg);

    ASSERT_TRUE(compatible);
}

TEST_F(CheckDependenciesTest, TestLinkedMinorGreater)
{
    LibraryVersion compiledVersion
    {
        1,
        1,
        0,
    };

    auto linkedVersion = [](unsigned int* major, unsigned int* minor, unsigned int* revision)
    {
        *major = 1;
        *minor = 2;
        *revision = 0;
    };

    StringPtr errMsg;
    bool compatible = isCompatibleVersion("CoreTypes", linkedVersion, compiledVersion, &errMsg);

    ASSERT_TRUE(compatible);
}
