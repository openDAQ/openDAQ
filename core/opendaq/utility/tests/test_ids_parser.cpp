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
