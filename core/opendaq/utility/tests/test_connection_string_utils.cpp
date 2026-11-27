#include <opendaq/connection_string_utils.h>
#include <gtest/gtest.h>

using ConnectionStringUtilsTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ConnectionStringUtilsTest, GetPort)
{
    ASSERT_EQ(GetConnectionStringPort("daq.nd://127.0.0.1:7420/").value_or(-1), 7420);
    ASSERT_EQ(GetConnectionStringPort("daq.nd://127.0.0.1:7420").value_or(-1), 7420);
    ASSERT_EQ(GetConnectionStringPort("daq.nd://host.local:7422/path?x=1").value_or(-1), 7422);
    ASSERT_EQ(GetConnectionStringPort("daq.nd://[::1]:7420/").value_or(-1), 7420);
}

TEST_F(ConnectionStringUtilsTest, GetPortWhenNoneIsNamed)
{
    ASSERT_FALSE(GetConnectionStringPort("daq.nd://127.0.0.1").has_value());
    ASSERT_FALSE(GetConnectionStringPort("daq.nd://127.0.0.1/path").has_value());
    ASSERT_FALSE(GetConnectionStringPort("daq.nd://[::1]/").has_value());
    ASSERT_FALSE(GetConnectionStringPort("daq.nd://fe80::1/").has_value());
    ASSERT_FALSE(GetConnectionStringPort("daq.nd://127.0.0.1:/").has_value());
    ASSERT_FALSE(GetConnectionStringPort("daq.nd://127.0.0.1:port/").has_value());
    ASSERT_FALSE(GetConnectionStringPort("daq://openDAQ_123").has_value());
    ASSERT_FALSE(GetConnectionStringPort("no-prefix").has_value());
    ASSERT_FALSE(GetConnectionStringPort(nullptr).has_value());
}

TEST_F(ConnectionStringUtilsTest, ReplacePort)
{
    ASSERT_EQ(ReplaceConnectionStringPort("daq.nd://127.0.0.1:7420/", 7422), "daq.nd://127.0.0.1:7422/");
    ASSERT_EQ(ReplaceConnectionStringPort("daq.nd://127.0.0.1", 7422), "daq.nd://127.0.0.1:7422");
    ASSERT_EQ(ReplaceConnectionStringPort("daq.nd://[::1]:7420/path", 7422), "daq.nd://[::1]:7422/path");
    ASSERT_EQ(ReplaceConnectionStringPort("daq.nd://fe80::1/", 7422), "daq.nd://fe80::1/");
}

END_NAMESPACE_OPENDAQ
