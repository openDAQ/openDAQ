#include <copendaq.h>

#include <gtest/gtest.h>

#include <opendaq/opendaq.h>
#include "opendaq/gmock/function_block.h"

class COpendaqFunctionBlockTest : public testing::Test
{
    void SetUp() override
    {
        EXPECT_CALL(fb.mock(), getFunctionBlockType(testing::_)).WillRepeatedly(daq::Get(daq::FunctionBlockType("test", "test", "test")));
    }

protected:
    daq::MockFunctionBlock::Strict fb;
};

TEST_F(COpendaqFunctionBlockTest, FunctionBlock)
{
    FunctionBlock* funcBlock = reinterpret_cast<FunctionBlock*>(fb->getObject());
    FunctionBlockType* fbType = nullptr;
    FunctionBlock_getFunctionBlockType(funcBlock, &fbType);
    ASSERT_NE(fbType, nullptr);
    BaseObject_releaseRef(fbType);
}
