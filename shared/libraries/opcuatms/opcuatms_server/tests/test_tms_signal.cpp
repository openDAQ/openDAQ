#include <opendaq/input_port_factory.h>
#include <open62541/daqbsp_nodeids.h>
#include <open62541/daqbt_nodeids.h>
#include <open62541/nodeids.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_ptr.h>
#include <gtest/gtest.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuatms_server/objects/tms_server_input_port.h>
#include <opcuatms_server/objects/tms_server_signal.h>
#include "tms_server_test.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsSignalTest : public TmsServerObjectTest
{
public:
    SignalConfigPtr createSignal(const ContextPtr& context, const StringPtr& localId = "sig")
    {
        SignalConfigPtr signal = Signal(context, nullptr, localId);
        signal.setActive(false);
        return signal;
    }
};

TEST_F(TmsSignalTest, Create)
{
    SignalConfigPtr signal = Signal(ctx, nullptr, "sig");
    auto tmsSignal = TmsServerSignal(signal, this->getServer(), ctx, tmsCtx);
}

TEST_F(TmsSignalTest, Register)
{
    SignalConfigPtr signal = Signal(ctx, nullptr, "sig");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, tmsCtx);
    auto nodeId = serverSignal.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
}

TEST_F(TmsSignalTest, DomainSignalReference)
{
    SignalConfigPtr signal = createSignal(ctx, "signal");
    SignalConfigPtr domainSignal = createSignal(ctx, "time signal");
    signal.setDomainSignal(domainSignal);

    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, tmsCtx);
    auto signalNodeId = serverSignal.registerOpcUaNode();

    auto serverDomainSignal = TmsServerSignal(domainSignal, this->getServer(), ctx, tmsCtx);
    auto domainSignalNodeId = serverDomainSignal.registerOpcUaNode();

    ASSERT_NO_THROW(serverSignal.createNonhierarchicalReferences());

    ASSERT_TRUE(this->getClient()->nodeExists(signalNodeId));
    ASSERT_TRUE(this->getClient()->nodeExists(domainSignalNodeId));

    OpcUaServerNode signalNode(*this->getServer(), signalNodeId);
    auto hasDomainNodes = signalNode.browse(OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL));
    ASSERT_EQ(hasDomainNodes.size(), 1u);
    ASSERT_EQ(hasDomainNodes[0]->getNodeId(), domainSignalNodeId);
}

TEST_F(TmsSignalTest, ValueAndAnalogValueDataType)
{
    // Create signal with Float64 descriptor
    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Float64)
                          .setName("TestSignal")
                          .build();
    SignalConfigPtr signal = createSignal(ctx, "signal");
    signal.setDescriptor(descriptor);

    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, tmsCtx);
    auto signalNodeId = serverSignal.registerOpcUaNode();

    // Get Value and AnalogValue node IDs
    OpcUaServerNode signalNode(*this->getServer(), signalNodeId);
    auto valueNodeId = getChildNodeId(signalNodeId, "Value");
    auto analogValueNodeId = getChildNodeId(signalNodeId, "AnalogValue");

    // Check that Value node has Float64 data type (not abstract NUMBER)
    auto valueDataType = this->getClient()->readDataType(valueNodeId);
    ASSERT_EQ(valueDataType, OpcUaNodeId(0, UA_NS0ID_DOUBLE));

    // Check that AnalogValue node has Float64 data type (not abstract NUMBER)
    auto analogValueDataType = this->getClient()->readDataType(analogValueNodeId);
    ASSERT_EQ(analogValueDataType, OpcUaNodeId(0, UA_NS0ID_DOUBLE));
}

TEST_F(TmsSignalTest, ValueAndAnalogValueDataTypeInt32)
{
    // Create signal with Int32 descriptor
    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Int32)
                          .setName("TestSignal")
                          .build();
    SignalConfigPtr signal = createSignal(ctx, "signal");
    signal.setDescriptor(descriptor);

    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, tmsCtx);
    auto signalNodeId = serverSignal.registerOpcUaNode();

    // Get Value and AnalogValue node IDs
    auto valueNodeId = getChildNodeId(signalNodeId, "Value");
    auto analogValueNodeId = getChildNodeId(signalNodeId, "AnalogValue");

    // Check that Value node has Int32 data type (not abstract NUMBER)
    auto valueDataType = this->getClient()->readDataType(valueNodeId);
    ASSERT_EQ(valueDataType, OpcUaNodeId(0, UA_NS0ID_INT32));

    // Check that AnalogValue node has Int32 data type (not abstract NUMBER)
    auto analogValueDataType = this->getClient()->readDataType(analogValueNodeId);
    ASSERT_EQ(analogValueDataType, OpcUaNodeId(0, UA_NS0ID_INT32));
}