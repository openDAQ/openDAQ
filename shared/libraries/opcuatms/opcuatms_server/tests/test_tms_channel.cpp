#include <opcuashared/opcuacommon.h>
#include <opcuatms/type_mappings.h>
#include <opendaq/instance_factory.h>
#include <coreobjects/unit_factory.h>
#include "coreobjects/property_object_factory.h"
#include "gtest/gtest.h"
#include "opendaq/mock/mock_channel_factory.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_server/objects/tms_server_channel.h"
#include "tms_object_test.h"
#include "open62541/tmsbsp_nodeids.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace daq::opcua::utils;

class TmsChannelTest : public TmsObjectTest
{
public:
    ChannelPtr createChannel()
    {
        const auto context = NullContext();
        return MockChannel(context, nullptr, "mockch");
    }
};

TEST_F(TmsChannelTest, Create)
{
    ChannelPtr channel = createChannel();
    auto tmsChannel = TmsServerChannel(channel, this->getServer(), NullContext());
}

TEST_F(TmsChannelTest, Register)
{
    ChannelPtr channel = createChannel();
    auto serverChannel = TmsServerChannel(channel, this->getServer(), NullContext());
    auto nodeId = serverChannel.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
}

TEST_F(TmsChannelTest, AttrFunctionBlockType)
{
    ChannelPtr channel = createChannel();
    auto serverChannel = TmsServerChannel(channel, this->getServer(), NullContext());

    auto nodeId = serverChannel.registerOpcUaNode();

    auto variant = readChildNode(nodeId, "FunctionBlockInfo");
    ASSERT_TRUE(variant.isType<UA_FunctionBlockInfoStructure>());

    UA_FunctionBlockInfoStructure type = variant.readScalar<UA_FunctionBlockInfoStructure>();

    ASSERT_EQ(ToStdString(type.id), "mock_ch");
    ASSERT_EQ(ToStdString(type.name), "mock_ch");
    ASSERT_EQ(ToStdString(type.description), "");
}

TEST_F(TmsChannelTest, BrowseSignals)
{
    ChannelPtr channel = createChannel();
    auto serverChannel = TmsServerChannel(channel, this->getServer(), NullContext());
    auto nodeId = serverChannel.registerOpcUaNode();

    OpcUaServerNode serverNodeFB(*this->getServer(), nodeId);
    auto signalServerNode = serverNodeFB.getChildNode(UA_QUALIFIEDNAME_ALLOC(NAMESPACE_TMSBSP, "Sig"));
    auto signalReferences = signalServerNode->browse(OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASVALUESIGNAL));
    ASSERT_EQ(signalReferences.size(), 10u);
}

TEST_F(TmsChannelTest, Property)
{
    // Build server channel
    auto serverChannel = createChannel();

    const auto sampleRateProp =
        FloatPropertyBuilder("SampleRate", 100.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    serverChannel.addProperty(sampleRateProp);

    auto tmsServerChannel = TmsServerChannel(serverChannel, this->getServer(), NullContext());
    auto nodeId = tmsServerChannel.registerOpcUaNode();

    auto sampleRateNodeId = this->getChildNodeId(nodeId, "SampleRate");
    ASSERT_FALSE(sampleRateNodeId.isNull());

    auto srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 100.0);

    serverChannel.setPropertyValue("SampleRate", 14.0);

    srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 14.0);

    this->getServer()->writeValue(sampleRateNodeId, OpcUaVariant(22.2));

    srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 22.2);
}
