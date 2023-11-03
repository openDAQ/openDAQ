#include <opcuashared/opcuacommon.h>
#include <opcuatms/type_mappings.h>
#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_factory.h>
#include "gtest/gtest.h"
#include <opendaq/mock/mock_fb_factory.h>
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_server/objects/tms_server_function_block.h"
#include "tms_object_test.h"
#include "open62541/tmsbsp_nodeids.h"
#include <coreobjects/unit_factory.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace daq::opcua::utils;

class TmsFunctionBlockTest : public TmsObjectTest
{
public:

    FunctionBlockPtr createFunctionBlock(const FunctionBlockTypePtr& type = FunctionBlockType("uid", "name", "desc"))
    {
        const auto context = NullContext();

        return MockFunctionBlock(type, context, nullptr, "mockfb");
    }
};

TEST_F(TmsFunctionBlockTest, Create)
{
    FunctionBlockPtr functionBlock = createFunctionBlock();
    auto tmsFunctionBlock = TmsServerFunctionBlock(functionBlock, this->getServer(), NullContext());
}

TEST_F(TmsFunctionBlockTest, Register)
{
    FunctionBlockPtr functionBlock = createFunctionBlock();
    auto serverFunctionBlock = TmsServerFunctionBlock(functionBlock, this->getServer(), NullContext());
    auto nodeId = serverFunctionBlock.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
}

TEST_F(TmsFunctionBlockTest, AttrFunctionBlockType)
{
    // Build functionBlock info:
    const FunctionBlockTypePtr type = FunctionBlockType("ID", "NAME", "DESCRIPTION");

    // Build server functionBlock
    auto serverFunctionBlock = createFunctionBlock(type);

    ASSERT_EQ(serverFunctionBlock.getFunctionBlockType(), type);

    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), NullContext());
    auto nodeId = tmsServerFunctionBlock.registerOpcUaNode();

    auto variant = readChildNode(nodeId, "FunctionBlockInfo");
    ASSERT_TRUE(variant.isType<UA_FunctionBlockInfoStructure>());

    UA_FunctionBlockInfoStructure tmsType = variant.readScalar<UA_FunctionBlockInfoStructure>();

    ASSERT_EQ(ToStdString(tmsType.id), "ID");
    ASSERT_EQ(ToStdString(tmsType.name), "NAME");
    ASSERT_EQ(ToStdString(tmsType.description), "DESCRIPTION");
}

TEST_F(TmsFunctionBlockTest, BrowseSignals)
{
    FunctionBlockPtr functionBlock = createFunctionBlock();
    auto serverFunctionBlock = TmsServerFunctionBlock(functionBlock, this->getServer(), NullContext());
    auto nodeId = serverFunctionBlock.registerOpcUaNode();

    OpcUaServerNode serverNodeFB(*this->getServer(), nodeId);
    auto signalServerNode = serverNodeFB.getChildNode(UA_QUALIFIEDNAME_ALLOC(NAMESPACE_TMSBSP, "Sig"));
    auto signalReferences = signalServerNode->browse(OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASVALUESIGNAL));
    ASSERT_EQ(signalReferences.size(), 5u);
}

TEST_F(TmsFunctionBlockTest, Property)
{
    // Build functionBlock info:
    const FunctionBlockTypePtr type = FunctionBlockType("uid", "name", "desc");

    // Build server functionBlock
    auto serverFunctionBlock = createFunctionBlock(type);

    const auto sampleRateProp =
        FloatPropertyBuilder("SampleRate", 100.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    serverFunctionBlock.addProperty(sampleRateProp);

    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), NullContext());
    auto nodeId = tmsServerFunctionBlock.registerOpcUaNode();

    auto sampleRateNodeId = this->getChildNodeId(nodeId, "SampleRate");
    ASSERT_FALSE(sampleRateNodeId.isNull());

    auto srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 100.0);

    serverFunctionBlock.setPropertyValue("SampleRate", 14.0);

    srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 14.0);

    this->getServer()->writeValue(sampleRateNodeId, OpcUaVariant(22.2));

    srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 22.2);
}

TEST_F(TmsFunctionBlockTest, NestedFunctionBlocks)
{
    FunctionBlockPtr functionBlock = createFunctionBlock();

    auto serverFunctionBlock = TmsServerFunctionBlock(functionBlock, this->getServer(), NullContext());    
    auto nodeId = serverFunctionBlock.registerOpcUaNode();

    auto firstFB = functionBlock.getFunctionBlocks()[0];
    auto firstFBNodeId = OpcUaNodeId(nodeId.getNamespaceIndex(), firstFB.getGlobalId().toStdString());
    ASSERT_TRUE(getServer()->nodeExists(firstFBNodeId));
}
