#include <opendaq/range_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_ptr.h>
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms/exceptions.h"
#include "opcuatms_client/objects/tms_client_signal_factory.h"
#include "opcuatms_server/objects/tms_server_signal.h"
#include "open62541/daqbsp_nodeids.h"
#include <opendaq/context_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/data_rule_factory.h>
#include "tms_object_integration_test.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsSignalTest : public TmsObjectIntegrationTest
{
public:
    SignalPtr createSignal(const std::string& id)
    {
        SignalPtr signal = Signal(NullContext(), nullptr, id);
        signal->setActive(false);
        return signal;
    }

    SignalPtr createFullSignal(const std::string& signalName)
    {
        // Build signal descriptor:
        auto serverMetadata = Dict<IString, IBaseObject>();
        for (size_t i = 0; i < 2; i++)
        {
            serverMetadata.set("Metadata" + std::to_string(i), "Value " + std::to_string(i));
        }

        // Build value descriptor:
        auto serverDataDescriptor = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Float32)
                                        .setName("Value Name")
                                        .setDimensions(List<IDimension>())
                                        .setRule(ConstantDataRule(1.0))
                                        .setUnit(Unit("symbol", 1, "name", "quantity"))  // Optional
                                        .setOrigin("Origin")                             // Optional
                                        .setValueRange(Range(0.0, 100.0))                // Optional
                                        .setMetadata(serverMetadata)
                                        .build();

        // Build server signal
        auto serverSignal = SignalWithDescriptor(NullContext(), serverDataDescriptor, nullptr, "sig");
        serverSignal->setActive(true);
        serverSignal.getTags().add("tag1");
        serverSignal.getTags().add("tag2");

        return serverSignal;
    }
};

TEST_F(TmsSignalTest, Create)
{
    SignalPtr signal = Signal(NullContext(), nullptr, "sig");
    auto tmsSignal = TmsServerSignal(signal, this->getServer(), NullContext());
}

TEST_F(TmsSignalTest, Register)
{
    SignalPtr signal = createSignal("id");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);
    ASSERT_TRUE(clientSignal.assigned());
}

TEST_F(TmsSignalTest, AttrUniqueId)
{
    SignalPtr daqServerSignal = createSignal("id");
    daqServerSignal.setActive(true);

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "id", clientContext, nodeId);

    // UniqueId is set by core, so only test is that it is transferred correctly:
    ASSERT_EQ(daqServerSignal.getLocalId(), clientSignal.getLocalId());
}

TEST_F(TmsSignalTest, AttrActive)
{
    SignalPtr daqServerSignal = createSignal("id");
    daqServerSignal.setActive(true);

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_TRUE(daqServerSignal.getActive());
    ASSERT_TRUE(clientSignal.getActive());

    daqServerSignal.setActive(false);

    ASSERT_FALSE(daqServerSignal.getActive());
    ASSERT_FALSE(clientSignal.getActive());

    clientSignal.setActive(true);

    ASSERT_TRUE(daqServerSignal.getActive());
    ASSERT_TRUE(clientSignal.getActive());
}

TEST_F(TmsSignalTest, AttrPublic)
{
    SignalPtr daqServerSignal = createSignal("id");

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_TRUE(clientSignal.getPublic());

    // client side change is reflected on client side (public is not transferred):
    clientSignal.setPublic(false);
    ASSERT_FALSE(clientSignal.getPublic());
    
    clientSignal.setPublic(true);
    ASSERT_TRUE(clientSignal.getPublic());
}

TEST_F(TmsSignalTest, AttrDescriptor)
{
    auto serverMetadata = Dict<IString, IBaseObject>();
    for (size_t i = 0; i < 2; i++)
    {
        serverMetadata.set("Metadata" + std::to_string(i), "Value " + std::to_string(i));
    }
    
    auto serverDataDescriptor = DataDescriptorBuilder()
                                    .setSampleType(SampleType::Float32)
                                    .setDimensions(List<IDimension>())
                                    .setRule(ConstantDataRule(1.0))
                                    .setUnit(Unit("symbol", 1, "name", "quantity"))  // Optional
                                    .setOrigin("Origin")                             // Optional
                                    .setValueRange(Range(0.0, 100.0))                // Optional
                                    .setName("Signal Name")
                                    .setMetadata(serverMetadata)
                                    .build();

    // Build server signal
    auto serverSignal = Signal(NullContext(), nullptr, "sig");
    serverSignal.setName("My signal");
    serverSignal.setDescription("My signal description");
    serverSignal->setActive(true);
    ASSERT_FALSE(serverSignal.getDescriptor().assigned());
    serverSignal.setDescriptor(serverDataDescriptor);
    ASSERT_EQ(serverSignal.getDescriptor(), serverDataDescriptor);

    auto tmsServerSignal = TmsServerSignal(serverSignal, this->getServer(), NullContext());
    auto nodeId = tmsServerSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_EQ(clientSignal.getActive(), serverSignal.getActive());
    // TODO: TMS signal should be implemented similar as fb, i.e. it needs to include property object
    //ASSERT_EQ(clientSignal.getName(), "My signal");
    // ASSERT_EQ(clientSignal.getDescription(), "My signal description");
    auto clientDataDescriptor = clientSignal.getDescriptor();
    ASSERT_EQ(clientDataDescriptor.getName(), "Signal Name");

    auto clientMetadata = clientDataDescriptor.getMetadata();
    auto keyList = clientMetadata.getKeyList();
    auto valueList = clientMetadata.getValueList();
    ASSERT_EQ(keyList.getCount(), 2u);
    ASSERT_EQ(valueList.getCount(), 2u);
    for (int i = 0; i < 2; i++)
    {
        auto key = keyList.getItemAt(i);
        auto value = valueList.getItemAt(i);
        ASSERT_EQ("Metadata" + std::to_string(i), key);
        ASSERT_EQ("Value " + std::to_string(i), value);
    }

    ASSERT_EQ(clientDataDescriptor.getDimensions().getCount(), 0u);
    ASSERT_EQ(clientDataDescriptor.getOrigin(), "Origin");
    ASSERT_EQ(clientDataDescriptor.getSampleType(), SampleType::Float32);

    auto clientRule = clientDataDescriptor.getRule();
    ASSERT_EQ(clientRule.getType(), DataRuleType::Constant);
    auto clientRuleParameters = clientRule.getParameters();
    ASSERT_EQ(clientRuleParameters.getCount(), 1u);
    auto clientKeyList = clientRuleParameters.getKeyList();
    auto clientValueList = clientRuleParameters.getValueList();
    auto k = clientKeyList.getItemAt(0);
    ASSERT_EQ(clientKeyList.getItemAt(0), "constant");
    auto v = clientValueList.getItemAt(0);
    ASSERT_EQ(clientValueList.getItemAt(0), 1.0);

    
    auto clientUnit = clientDataDescriptor.getUnit();
    ASSERT_EQ(clientUnit.getQuantity(), "quantity");
    ASSERT_EQ(clientUnit.getName(), "name");
    ASSERT_EQ(clientUnit.getSymbol(), "symbol");

    auto clientValueRange = clientDataDescriptor.getValueRange();
    ASSERT_EQ(clientValueRange.getLowValue(), 0.0);
    ASSERT_EQ(clientValueRange.getHighValue(), 100.0);

    auto browseName = client->readBrowseName(nodeId);
    ASSERT_EQ(browseName, "sig");

    auto displayName = client->readDisplayName(nodeId);
    ASSERT_EQ(displayName, "My signal");
}

TEST_F(TmsSignalTest, AttrDomainSignal)
{
    SignalPtr daqServerDomainSignal = createSignal("sig1");
    auto serverDomainSignal = TmsServerSignal(daqServerDomainSignal, this->getServer(), NullContext());
    auto domainNodeId = serverDomainSignal.registerOpcUaNode();

    SignalPtr daqServerSignal = createSignal("sig2");
    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    OpcUaNodeId referenceTypeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL);
    getServer()->addReference(nodeId, referenceTypeId, domainNodeId);
    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    [[maybe_unused]]
    SignalPtr clientDomainSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, domainNodeId);

    SignalPtr domainSignal;
    EXPECT_NO_THROW(domainSignal = clientSignal.getDomainSignal());
    ASSERT_EQ(clientDomainSignal, domainSignal);
}

TEST_F(TmsSignalTest, AttrRelatedSignals)
{
    SignalPtr daqServerSignal1 = createSignal("id1");
    auto serverSignal1 = TmsServerSignal(daqServerSignal1, this->getServer(), NullContext());
    auto nodeId1 = serverSignal1.registerOpcUaNode();

    SignalPtr daqServerSignal2 = createSignal("id2");
    auto serverSignal2 = TmsServerSignal(daqServerSignal2, this->getServer(), NullContext());
    auto nodeId2 = serverSignal2.registerOpcUaNode();

    SignalPtr daqServerSignal3 = createSignal("id3");
    auto serverSignal3 = TmsServerSignal(daqServerSignal3, this->getServer(), NullContext());
    auto nodeId3 = serverSignal3.registerOpcUaNode();

    OpcUaNodeId referenceTypeId(NAMESPACE_DAQBSP, UA_DAQBSPID_RELATESTOSIGNAL);
    getServer()->addReference(nodeId2, referenceTypeId, nodeId1);
    getServer()->addReference(nodeId3, referenceTypeId, nodeId1);
    getServer()->addReference(nodeId3, referenceTypeId, nodeId2);

    SignalPtr clientSignal1 = TmsClientSignal(NullContext(), nullptr, "sig1", clientContext, nodeId1);
    SignalPtr clientSignal2 = TmsClientSignal(NullContext(), nullptr, "sig2", clientContext, nodeId2);
    SignalPtr clientSignal3 = TmsClientSignal(NullContext(), nullptr, "sig3", clientContext, nodeId3);

    auto relatedSignals1 = clientSignal1.getRelatedSignals();
    auto relatedSignals2 = clientSignal2.getRelatedSignals();
    auto relatedSignals3 = clientSignal3.getRelatedSignals();

    ASSERT_TRUE(relatedSignals1 != nullptr);
    ASSERT_TRUE(relatedSignals2 != nullptr);
    ASSERT_TRUE(relatedSignals3 != nullptr);

    ASSERT_EQ(relatedSignals1.getCount(), 0u);

    ASSERT_EQ(relatedSignals2.getCount(), 1u);
    ASSERT_EQ(relatedSignals2[0].getLocalId(), "sig1");

    ASSERT_EQ(relatedSignals3.getCount(), 2u);
    // Cannot depend on order...
    ASSERT_TRUE((relatedSignals3[0] == clientSignal1 || relatedSignals3[0] == clientSignal2) && relatedSignals3[0] != clientSignal3);
    ASSERT_TRUE((relatedSignals3[1] == clientSignal1 || relatedSignals3[1] == clientSignal2) && relatedSignals3[1] != clientSignal3);
    ASSERT_TRUE(relatedSignals3[0] != relatedSignals3[1]);
}

TEST_F(TmsSignalTest, MethodGetConnections)
{
    SignalPtr daqServerSignal1 = createSignal("id");
    auto serverSignal1 = TmsServerSignal(daqServerSignal1, this->getServer(), NullContext());
    auto nodeId1 = serverSignal1.registerOpcUaNode();

    SignalPtr clientSignal1 = TmsClientSignal(NullContext(), nullptr, "sig",     clientContext, nodeId1);
    ASSERT_EQ(clientSignal1.getConnections().getCount(), 0u);

    //TODO: Implement more test when related signals actually can be returned
   //ASSERT_EQ(relatedSignals1.getCount(), 2u);
    //ASSERT_EQ(relatedSignals2.getCount(), 2u);
    //ASSERT_EQ(relatedSignals3.getCount(), 2u);
}

TEST_F(TmsSignalTest, ComponentMethods)
{
    auto signal = createFullSignal("sig");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_EQ(signal.getName(), clientSignal.getName());

    // TODO: Support changing read-only properties over OPC UA
    ASSERT_NO_THROW(clientSignal.setName("new_name"));
    ASSERT_EQ(signal.getName(), clientSignal.getName());

    ASSERT_NO_THROW(clientSignal.setDescription("new_description"));
    ASSERT_EQ(signal.getDescription(), clientSignal.getDescription());

    auto tags = signal.getTags();
    auto clientTags = clientSignal.getTags();
    ASSERT_TRUE(clientTags.query("tag1") && clientTags.query("tag2"));
}

TEST_F(TmsSignalTest, ExplicitDomainRuleDescriptor)
{
    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(ExplicitDomainDataRule(5.1, 20)).build();
    const auto signal = SignalWithDescriptor(NullContext(), descriptor, nullptr, "sig");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_EQ(signal.getDescriptor().getRule(), clientSignal.getDescriptor().getRule());
}
