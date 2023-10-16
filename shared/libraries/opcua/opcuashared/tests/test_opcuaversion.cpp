#include "gtest/gtest.h"
#include "opcuashared/opcuaversion.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaVersionTest = testing::Test;

TEST_F(OpcUaVersionTest, CreateDefault)
{
    OpcUaVersion version;
    ASSERT_EQ(version.major, 0);
    ASSERT_EQ(version.minor, 0);
    ASSERT_EQ(version.patch, 0);

    ASSERT_EQ(version.toString(), "0.0.0");
}

TEST_F(OpcUaVersionTest, Create)
{
    OpcUaVersion version(10, 20, 30);
    ASSERT_EQ(version.major, 10);
    ASSERT_EQ(version.minor, 20);
    ASSERT_EQ(version.patch, 30);

    ASSERT_EQ(version.toString(), "10.20.30");
}

TEST_F(OpcUaVersionTest, CreateByMembers)
{
    OpcUaVersion version{10, 20};
    ASSERT_EQ(version.major, 10);
    ASSERT_EQ(version.minor, 20);
    ASSERT_EQ(version.patch, 0);

    ASSERT_EQ(version.toString(), "10.20.0");
}

TEST_F(OpcUaVersionTest, CreateFromString)
{
    OpcUaVersion version("10.20.30");
    ASSERT_EQ(version.major, 10);
    ASSERT_EQ(version.minor, 20);
    ASSERT_EQ(version.patch, 30);

    ASSERT_EQ(version.toString(), "10.20.30");
}

TEST_F(OpcUaVersionTest, CreateFromErrString)
{
    OpcUaVersion version("dummy");
    ASSERT_EQ(version.major, 0);
    ASSERT_EQ(version.minor, 0);
    ASSERT_EQ(version.patch, 0);

    ASSERT_EQ(version.toString(), "0.0.0");
}

TEST_F(OpcUaVersionTest, LongestToString)
{
    OpcUaVersion version((std::numeric_limits<int>::min)(), (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::min)());

    ASSERT_EQ(version.toString(), "-2147483648.-2147483648.-2147483648");
}

TEST_F(OpcUaVersionTest, Compatible)
{
    OpcUaVersion v1(1, 1, 1);
    OpcUaVersion v2(1, 1, 2);
    OpcUaVersion v3(1, 2, 1);
    OpcUaVersion v4(3, 2, 1);

    ASSERT_TRUE(OpcUaVersion::Compatible(v1, v2));
    ASSERT_TRUE(OpcUaVersion::Compatible(v2, v1));

    ASSERT_TRUE(OpcUaVersion::Compatible(v1, v3));
    ASSERT_FALSE(OpcUaVersion::Compatible(v3, v1));

    ASSERT_FALSE(OpcUaVersion::Compatible(v1, v4));
    ASSERT_FALSE(OpcUaVersion::Compatible(v4, v1));
}

TEST_F(OpcUaVersionTest, HasFeature)
{
    OpcUaVersion featureVersion(2, 4, 0);

    OpcUaVersion serverVersion1(1, 6, 1);
    OpcUaVersion serverVersion2(2, 2, 2);
    OpcUaVersion serverVersion3(2, 4, 5);
    OpcUaVersion serverVersion4(2, 6, 1);
    OpcUaVersion serverVersion5(3, 6, 1);

    ASSERT_FALSE(OpcUaVersion::HasFeature(serverVersion1, featureVersion));
    ASSERT_FALSE(OpcUaVersion::HasFeature(serverVersion2, featureVersion));
    ASSERT_TRUE(OpcUaVersion::HasFeature(serverVersion3, featureVersion));
    ASSERT_TRUE(OpcUaVersion::HasFeature(serverVersion4, featureVersion));
    ASSERT_TRUE(OpcUaVersion::HasFeature(serverVersion5, featureVersion)); // it has feature, but u can remove backword compatibility code
}

END_NAMESPACE_OPENDAQ_OPCUA
