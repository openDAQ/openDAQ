#include <gtest/gtest.h>
#include <opendaq/ids_parser.h>

using namespace daq;

using IdsParserTest = testing::Test;

TEST_F(IdsParserTest, SplitValid)
{
    std::string start, rest;
    ASSERT_TRUE(IdsParser::splitRelativeId("xch/0/1/2", start, rest));
    ASSERT_EQ(start, "xch");
    ASSERT_EQ(rest, "0/1/2");
}

TEST_F(IdsParserTest, SplitInvalid)
{
    std::string start, rest;
    ASSERT_FALSE(IdsParser::splitRelativeId("xch", start, rest));
}

TEST_F(IdsParserTest, NestedId)
{
    ASSERT_FALSE(IdsParser::isNestedComponentId("xch", "xch_1/id"));
    ASSERT_TRUE(IdsParser::isNestedComponentId("xch", "xch/id"));
}

TEST_F(IdsParserTest, idEndsWith)
{
    ASSERT_FALSE(IdsParser::idEndsWith("x/0/1/2", "x"));
    ASSERT_FALSE(IdsParser::idEndsWith("x/0/1/2", "x/0/1/3"));
    ASSERT_TRUE(IdsParser::idEndsWith("x/0/1/2", "/1/2"));
    ASSERT_TRUE(IdsParser::idEndsWith("x/0/1/2", "x/0/1/2"));
}
