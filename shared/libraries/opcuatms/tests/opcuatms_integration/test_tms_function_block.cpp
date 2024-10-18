#include <opendaq/context_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/function_block_type.h>
#include <opendaq/function_block_type_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/input_port_factory.h>
#include <gtest/gtest.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuatms/exceptions.h>
#include <opcuatms_server/objects/tms_server_function_block.h>
#include <opcuatms_client/objects/tms_client_function_block_factory.h>
#include <opcuatms_client/objects/tms_client_input_port_factory.h>
#include <opcuatms_client/objects/tms_client_signal_factory.h>
#include <open62541/daqbsp_nodeids.h>
#include "tms_object_integration_test.h"
#include <opendaq/folder_config_ptr.h>

#include <opendaq/mock/mock_fb_factory.h>
#include <opendaq/mock/default_mocks_factory.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsFunctionBlockTest : public TmsObjectIntegrationTest
{
public:
    FunctionBlockPtr createFunctionBlock(const FunctionBlockTypePtr& type = FunctionBlockType("UID", "Name", "Desc"))
    {
        const auto context = NullContext();
        return MockFunctionBlock(type, context, nullptr, "mockfb");
    }
};

TEST_F(TmsFunctionBlockTest, Create)
{
    FunctionBlockPtr functionBlock = createFunctionBlock();
    auto tmsFunctionBlock = TmsServerFunctionBlock(functionBlock, this->getServer(), ctx, serverContext);
}

TEST_F(TmsFunctionBlockTest, Register)
{
    FunctionBlockPtr functionBlock = createFunctionBlock();
    auto serverFunctionBlock = TmsServerFunctionBlock(functionBlock, this->getServer(), ctx, serverContext);  // Not possible either
    auto nodeId = serverFunctionBlock.registerOpcUaNode();

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, nodeId);
    ASSERT_TRUE(clientFunctionBlock.assigned());
}

TEST_F(TmsFunctionBlockTest, BrowseName)
{
    // Build functionBlock info:
    const FunctionBlockTypePtr type = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");

    // Build server functionBlock
    auto serverFunctionBlock = createFunctionBlock(type);
   
    ASSERT_EQ(serverFunctionBlock.getFunctionBlockType(), type);
    
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerFunctionBlock.registerOpcUaNode();

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, nodeId);

    auto browseName = client->readBrowseName(nodeId);
    ASSERT_EQ(browseName, "mockfb");

    auto displayName = client->readDisplayName(nodeId);
    ASSERT_EQ(displayName, "mockfb");
}

TEST_F(TmsFunctionBlockTest, AttrFunctionBlockType)
{
    // Build functionBlock info:
    const FunctionBlockTypePtr type = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");

    // Build server functionBlock
    auto serverFunctionBlock = createFunctionBlock(type);
   
    ASSERT_EQ(serverFunctionBlock.getFunctionBlockType(), type);
    
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerFunctionBlock.registerOpcUaNode();

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, nodeId);

    auto clientType = clientFunctionBlock.getFunctionBlockType();
    ASSERT_EQ(clientType.getId(), "UNIQUE ID");
    ASSERT_EQ(clientType.getName(), "NAME");
    ASSERT_EQ(clientType.getDescription(), "DESCRIPTION");
}

TEST_F(TmsFunctionBlockTest, MethodGetInputPorts)
{
    const FunctionBlockTypePtr type = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");
    auto serverFunctionBlock = createFunctionBlock(type);
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);

    auto inputPorts = clientFunctionBlock.getInputPorts();
    ASSERT_TRUE(inputPorts != nullptr);

    ASSERT_EQ(inputPorts.getCount(), 2u); // Mock function block creates 2 input ports
    ASSERT_EQ(inputPorts[0].getLocalId(), "TestInputPort1");
    ASSERT_EQ(inputPorts[1].getLocalId(), "TestInputPort2");
}

TEST_F(TmsFunctionBlockTest, MethodGetSignals)
{
    const FunctionBlockTypePtr type = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");
    auto serverFunctionBlock = createFunctionBlock(type);
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();

    auto clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);

    ListPtr<ISignal> serverSignals = serverFunctionBlock.getSignals();
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientFunctionBlock.getSignals());
    ASSERT_TRUE(signals.assigned());

    ASSERT_EQ(signals.getCount(), 4u);
    ASSERT_EQ(signals[0].getDescriptor().getName(), "Signal1");
    ASSERT_EQ(signals[1].getDescriptor().getName(), "Signal2");
    ASSERT_EQ(signals[2].getDescriptor().getName(), "Signal3");
    ASSERT_EQ(signals[3].getDescriptor().getName(), "Signal4");
}

TEST_F(TmsFunctionBlockTest, SignalCheckGlobalId)
{
    const FunctionBlockTypePtr type = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");
    auto serverFunctionBlock = createFunctionBlock(type);
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();

    auto clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, serverFunctionBlock.getLocalId(), clientContext, functionBlockNodeId);

    ListPtr<ISignal> serverSignals = serverFunctionBlock.getSignals();
    ListPtr<ISignal> clientSignals = clientFunctionBlock.getSignals();

    // one private signal in MockPhysicalDeviceImpl
    ASSERT_EQ(clientSignals.getCount(), serverSignals.getCount() - 1);

    std::vector<std::string> serverSignalsName;
    for (const auto & signal : serverSignals)
        serverSignalsName.push_back(signal.getGlobalId());

    for (const auto & signal : clientSignals)
    {
        auto it = find(serverSignalsName.begin(), serverSignalsName.end(), signal.getGlobalId().toStdString());
        ASSERT_NE(it, serverSignalsName.end());
    }
}

TEST_F(TmsFunctionBlockTest, MethodGetStatusSignal)
{
    const FunctionBlockTypePtr type = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");
    auto serverFunctionBlock = createFunctionBlock(type);
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();

    SignalPtr serverSignal = Signal(NullContext(), nullptr, "sig");
    auto tmsServerSignal = TmsServerSignal(serverSignal, this->getServer(), ctx, serverContext);
    auto signalNodeId = tmsServerSignal.registerOpcUaNode();

    OpcUaNodeId referenceTypeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASSTATUSSIGNAL);
    getServer()->addReference(functionBlockNodeId, referenceTypeId, signalNodeId, true);

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);
    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, signalNodeId);

    auto statusSignal = clientFunctionBlock.getStatusSignal();
    ASSERT_EQ(statusSignal, clientSignal);    
}

TEST_F(TmsFunctionBlockTest, Property)
{
    const FunctionBlockTypePtr serverFunctionBlockType = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");
    auto serverFunctionBlock = createFunctionBlock(serverFunctionBlockType);
    
    const auto sampleRateProp = FloatPropertyBuilder("SampleRate", 100.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    serverFunctionBlock.addProperty(sampleRateProp);

    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerFunctionBlock.registerOpcUaNode();

    auto clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, nodeId);

    auto visibleProperties = clientFunctionBlock.getVisibleProperties();
    ASSERT_EQ(visibleProperties.getCount(), 3u);
    ASSERT_EQ(visibleProperties[0].getName(), "TestConfigInt");
    ASSERT_EQ(visibleProperties[1].getName(), "TestConfigString");
    ASSERT_EQ(visibleProperties[2].getName(), "SampleRate");

    auto properties = clientFunctionBlock.getAllProperties();
    ASSERT_EQ(properties.getCount(), 3u);
    ASSERT_EQ(properties[0].getName(), "TestConfigInt");
    ASSERT_EQ(properties[1].getName(), "TestConfigString");
    ASSERT_EQ(properties[2].getName(), "SampleRate");

    ASSERT_TRUE(clientFunctionBlock.hasProperty("SampleRate"));
    ASSERT_EQ(clientFunctionBlock.getPropertyValue("SampleRate"), 100.0);

    clientFunctionBlock.setPropertyValue("SampleRate", 14.0);
    ASSERT_EQ(clientFunctionBlock.getPropertyValue("SampleRate"), 14.0);
    ASSERT_EQ(serverFunctionBlock.getPropertyValue("SampleRate"), 14.0);
}

TEST_F(TmsFunctionBlockTest, NestedFunctionBlocks)
{
    auto serverFunctionBlock = createFunctionBlock();
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);
    ASSERT_NO_THROW(clientFunctionBlock.getFunctionBlocks());
    auto nestedFunctionBlocks = clientFunctionBlock.getFunctionBlocks();
    ASSERT_EQ(nestedFunctionBlocks.getCount(), 1u);
}

TEST_F(TmsFunctionBlockTest, ComponentMethods)
{
    auto serverFunctionBlock = createFunctionBlock();
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();

    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);
    
    ASSERT_EQ(serverFunctionBlock.getName(), clientFunctionBlock.getName());

    auto tags = serverFunctionBlock.getTags();
    auto clientTags = clientFunctionBlock.getTags();

    ASSERT_TRUE(tags.query("mock_fb"));
    ASSERT_TRUE(clientTags.query("mock_fb"));

    clientFunctionBlock.setActive(false);
    ASSERT_EQ(serverFunctionBlock.getActive(), clientFunctionBlock.getActive());

    clientFunctionBlock.setActive(true);
    ASSERT_EQ(serverFunctionBlock.getActive(), clientFunctionBlock.getActive());
}

TEST_F(TmsFunctionBlockTest, SignalOrder)
{
    auto serverFunctionBlock = DefaultFunctionBlock(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverFunctionBlock.getItem("Sig");
    for (int i = 0; i < 100; ++i)
        folder.addItem(Signal(NullContext(), folder, "sig_" + std::to_string(i)));
    
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();
    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);

    const auto serverSignals = serverFunctionBlock.getSignals();
    const auto clientSignals = clientFunctionBlock.getSignals();

    for (SizeT i = 0; i < serverSignals.getCount(); ++i)
        ASSERT_EQ(serverSignals[i].getName(), clientSignals[i].getName());
}

TEST_F(TmsFunctionBlockTest, InputPortOrder)
{
    auto serverFunctionBlock = DefaultFunctionBlock(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverFunctionBlock.getItem("IP");
    for (int i = 0; i < 100; ++i)
        folder.addItem(InputPort(NullContext(), folder, "ip_" + std::to_string(i)));
    
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();
    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);

    const auto serverInputPorts = serverFunctionBlock.getInputPorts();
    const auto clientInputPorts = clientFunctionBlock.getInputPorts();

    for (SizeT i = 0; i < serverInputPorts.getCount(); ++i)
        ASSERT_EQ(serverInputPorts[i].getName(), clientInputPorts[i].getName());
}

TEST_F(TmsFunctionBlockTest, FunctionBlockOrder)
{
    auto serverFunctionBlock = DefaultFunctionBlock(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverFunctionBlock.getItem("FB");
    for (int i = 0; i < 40; ++i)
        folder.addItem(DefaultFunctionBlock(NullContext(), folder, "fb_" + std::to_string(i)));
    
    auto tmsServerFunctionBlock = TmsServerFunctionBlock(serverFunctionBlock, this->getServer(), ctx, serverContext);
    auto functionBlockNodeId = tmsServerFunctionBlock.registerOpcUaNode();
    FunctionBlockPtr clientFunctionBlock = TmsClientFunctionBlock(NullContext(), nullptr, "mockfb", clientContext, functionBlockNodeId);

    const auto serverFunctionBlocks = serverFunctionBlock.getFunctionBlocks();
    const auto clientFunctionBlocks = clientFunctionBlock.getFunctionBlocks();

    for (SizeT i = 0; i < serverFunctionBlocks.getCount(); ++i)
        ASSERT_EQ(serverFunctionBlocks[i].getName(), clientFunctionBlocks[i].getName());
}
