#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_ptr.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_type_factory.h>

using FunctionBlockTest = testing::Test;

TEST_F(FunctionBlockTest, Folders)
{
    daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("test_uid", "test_name", "test_description");
    auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(fbType, daq::NullContext(), nullptr, "fb");

    ASSERT_EQ(fb.getSignals().getElementInterfaceId(), daq::ISignal::Id);
    ASSERT_EQ(fb.getFunctionBlocks().getElementInterfaceId(), daq::IFunctionBlock::Id);
    ASSERT_EQ(fb.getInputPorts().getElementInterfaceId(), daq::IInputPort::Id);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructType)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockType("id", "name", "desc");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructFields)
{
    const daq::StructPtr structPtr = daq::FunctionBlockType("id", "name", "desc");
    ASSERT_EQ(structPtr.get("id"), "id");
    ASSERT_EQ(structPtr.get("name"), "name");
    ASSERT_EQ(structPtr.get("description"), "desc");
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructNames)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockType("id", "name", "desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(FunctionBlockTest, HasItem)
{
    daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("test_uid", "test_name", "test_description");
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(fbType, daq::NullContext(), nullptr, "fb");

    ASSERT_TRUE(fb.hasItem("sig"));
    ASSERT_TRUE(fb.hasItem("ip"));
    ASSERT_TRUE(fb.hasItem("fb"));
    ASSERT_FALSE(fb.hasItem("none"));
}
