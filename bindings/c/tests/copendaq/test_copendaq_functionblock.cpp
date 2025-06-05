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
    daqFunctionBlock* funcBlock = (daqFunctionBlock*) fb->getObject();
    daqFunctionBlockType* fbType = nullptr;
    daqFunctionBlock_getFunctionBlockType(funcBlock, &fbType);
    ASSERT_NE(fbType, nullptr);
    daqBaseObject_releaseRef(fbType);
}
