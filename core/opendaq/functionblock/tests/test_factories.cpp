#include <opendaq/function_block_type_factory.h>
#include <opendaq/signal_factory.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>

using FunctionBlockFactoriesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(FunctionBlockFactoriesTest, TestFunctionBlockTypeFactory)
{
    FunctionBlockTypePtr fb;
    ASSERT_NO_THROW(fb = FunctionBlockType("test_uid", "test_name", "test_description"));
    ASSERT_EQ(fb.getId(), "test_uid");
    ASSERT_EQ(fb.getName(), "test_name");
    ASSERT_EQ(fb.getDescription(), "test_description");
}

END_NAMESPACE_OPENDAQ
