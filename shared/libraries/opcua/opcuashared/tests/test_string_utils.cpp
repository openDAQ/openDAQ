#include <gtest/gtest.h>
#include <opcuashared/opcuacommon.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaStringTest = testing::Test;

TEST_F(OpcUaStringTest, Compare)
{
    UA_String first = UA_STRING_STATIC("a");
    UA_String second = UA_STRING_STATIC("b");
    UA_String third = UA_STRING_STATIC("a");

    ASSERT_TRUE(first == third);
    ASSERT_FALSE(first == second);

    ASSERT_FALSE(first != third);
    ASSERT_TRUE(first != second);
}

TEST_F(OpcUaStringTest, CompareConstChar)
{
    UA_String first = UA_STRING_STATIC("a");
    ASSERT_TRUE(first == "a");
    ASSERT_FALSE(first != "a");
    ASSERT_FALSE(first == "b");
    ASSERT_TRUE(first != "b");

    ASSERT_TRUE("a" == first);
    ASSERT_FALSE("a" != first);
    ASSERT_FALSE("b" == first);
    ASSERT_TRUE("b" != first);
}

TEST_F(OpcUaStringTest, ToStdString)
{
    UA_String test = UA_STRING_STATIC("test");
    std::string testStr = utils::ToStdString(test);
    ASSERT_EQ(testStr, "test");
}

TEST_F(OpcUaStringTest, ToStdStringNull)
{
    UA_String test{};
    std::string testStr = utils::ToStdString(test);
    ASSERT_EQ(testStr, "");
}


END_NAMESPACE_OPENDAQ_OPCUA
