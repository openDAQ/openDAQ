#include <opendaq/context_factory.h>
#include <opendaq/channel_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/input_port_factory.h>
#include <gtest/gtest.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuatms/exceptions.h>
#include <opcuatms_server/objects/tms_server_channel.h>
#include <opcuatms_client/objects/tms_client_channel_factory.h>
#include <opcuatms_client/objects/tms_client_input_port_factory.h>
#include <opcuatms_client/objects/tms_client_signal_factory.h>
#include <open62541/daqbsp_nodeids.h>
#include "tms_object_integration_test.h"
#include <opendaq/mock/mock_channel_factory.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
class TmsChannelTest : public TmsObjectIntegrationTest
{
public:

    ChannelPtr createChannel(const ContextPtr& ctx)
    {
        return MockChannel(ctx, nullptr, "mockch");
    }
};

TEST_F(TmsChannelTest, Create)
{
    ChannelPtr channel = createChannel(ctx);
    auto tmsChannel = TmsServerChannel(channel, this->getServer(), ctx, serverContext);
}

TEST_F(TmsChannelTest, Register)
{
    auto ctx = NullContext();
    ChannelPtr channel = createChannel(ctx);
    auto serverChannel = TmsServerChannel(channel, this->getServer(), ctx, serverContext);  // Not possible either
    auto nodeId = serverChannel.registerOpcUaNode();

    ChannelPtr clientChannel = TmsClientChannel(ctx, nullptr, "Ch", clientContext, nodeId);
    ASSERT_TRUE(clientChannel.assigned());
}

TEST_F(TmsChannelTest, BrowseName)
{
    auto ctx = NullContext();
    ChannelPtr serverChannel = createChannel(ctx);
    
    auto tmsServerChannel = TmsServerChannel(serverChannel, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerChannel.registerOpcUaNode();

    ChannelPtr clientChannel = TmsClientChannel(ctx, nullptr, "Ch", clientContext, nodeId);

    auto browseName = client->readBrowseName(nodeId);
    ASSERT_EQ(browseName, "mockch");

    auto dsiplayName = client->readDisplayName(nodeId);
    ASSERT_EQ(dsiplayName, "mockch");
}

TEST_F(TmsChannelTest, AttrFunctionBlockType)
{
    auto ctx = NullContext();
    ChannelPtr serverChannel = createChannel(ctx);

    auto tmsServerChannel = TmsServerChannel(serverChannel, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerChannel.registerOpcUaNode();

    ChannelPtr clientChannel = TmsClientChannel(ctx, nullptr, "Ch", clientContext, nodeId);

    auto type = clientChannel.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "mock_ch");
    ASSERT_EQ(type.getName(), "mock_ch");
    ASSERT_EQ(type.getDescription(), "");
}

TEST_F(TmsChannelTest, MethodGetInputPorts)
{
    auto ctx = NullContext();
    ChannelPtr serverChannel = createChannel(ctx);

    auto tmsServerChannel = TmsServerChannel(serverChannel, this->getServer(), ctx, serverContext);
    auto channelNodeId = tmsServerChannel.registerOpcUaNode();

    ChannelPtr clientChannel = TmsClientChannel(ctx, nullptr, "Ch", clientContext, channelNodeId);

    auto inputPorts = clientChannel.getInputPorts();
    ASSERT_TRUE(inputPorts != nullptr);

    //TODO: Implement more test when input ports actually can be returned
    ASSERT_EQ(inputPorts.getCount(), 2u); // Mock function block creates 2 input ports
    ASSERT_EQ(inputPorts[0].getLocalId(), "TestInputPort1");
    ASSERT_EQ(inputPorts[1].getLocalId(), "TestInputPort2");
}

TEST_F(TmsChannelTest, MethodGetSignals)
{
    auto ctx = NullContext();
    ChannelPtr serverChannel = createChannel(ctx);

    auto tmsServerChannel = TmsServerChannel(serverChannel, this->getServer(), ctx, serverContext);
    auto channelNodeId = tmsServerChannel.registerOpcUaNode();

    auto clientChannel = TmsClientChannel(ctx, nullptr, "Ch", clientContext, channelNodeId);

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientChannel.getSignals());
    ASSERT_TRUE(signals.assigned());

    ASSERT_EQ(signals.getCount(), 10u);
    ASSERT_EQ(signals[0].getDescriptor().getName(), "Signal1");
    ASSERT_EQ(signals[1].getDescriptor().getName(), "Signal2");
    ASSERT_EQ(signals[2].getDescriptor().getName(), "Signal3");
    ASSERT_EQ(signals[3].getDescriptor().getName(), "Signal4");
    ASSERT_EQ(signals[4].getDescriptor().getName(), "Time");
    ASSERT_EQ(signals[5].getDescriptor().getName(), "ChangingTime");
    ASSERT_EQ(signals[6].getDescriptor().getName(), "ByteStep");
    ASSERT_EQ(signals[7].getDescriptor().getName(), "IntStep");
    ASSERT_EQ(signals[8].getDescriptor().getName(), "Sine");
    ASSERT_EQ(signals[9].getDescriptor().getName(), "ChangingSignal");
}

TEST_F(TmsChannelTest, MethodGetStatusSignal)
{
    auto ctx = NullContext();
    ChannelPtr serverChannel = createChannel(ctx);

    auto tmsServerChannel = TmsServerChannel(serverChannel, this->getServer(), ctx, serverContext);
    auto channelNodeId = tmsServerChannel.registerOpcUaNode();

    SignalPtr serverSignal = Signal(ctx, nullptr, "sig");
    auto tmsServerSignal = TmsServerSignal(serverSignal, this->getServer(), ctx, serverContext);
    auto signalNodeId = tmsServerSignal.registerOpcUaNode();

    OpcUaNodeId referenceTypeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASSTATUSSIGNAL);
    getServer()->addReference(channelNodeId, referenceTypeId, signalNodeId, true);

    ChannelPtr clientChannel = TmsClientChannel(ctx, nullptr, "Ch", clientContext, channelNodeId);

    //EXPECT_THROW(clientChannel.getStatusSignal(), daq::opcua::OpcUaClientCallNotAvailableException);

    //TODO: Implement more test when signals actually can be returned
    //SignalPtr clientSignal = TmsClientSignal(client, signalNodeId);
    //auto statusSignal = clientChannel.getStatusSignal();
    //ASSERT_EQ(statusSignal, clientSignal);    
}
