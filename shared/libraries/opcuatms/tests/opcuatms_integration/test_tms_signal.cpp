#include <coreobjects/unit_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/core_opendaq_event_args_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <gtest/gtest.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuaclient/cached_reference_browser.h>
#include <opcuatms/exceptions.h>
#include <opcuatms_client/objects/tms_client_signal_factory.h>
#include <opcuatms_server/objects/tms_server_signal.h>
#include <open62541/daqbsp_nodeids.h>
#include <open62541/nodeids.h>
#include <thread>
#include <chrono>
#include <memory>
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
                                        .setRule(ConstantDataRule())
                                        .setUnit(Unit("Symbol", 1, "Name", "Quantity"))  // Optional
                                        .setOrigin("Origin")                             // Optional
                                        .setValueRange(Range(0.0, 100.0))                // Optional
                                        .setMetadata(serverMetadata)
                                        .build();

        // Build server signal
        auto serverSignal = SignalWithDescriptor(NullContext(), serverDataDescriptor, nullptr, "sig");
        serverSignal->setActive(true);
        serverSignal.getTags().asPtr<ITagsPrivate>().add("tag1");
        serverSignal.getTags().asPtr<ITagsPrivate>().add("tag2");

        return serverSignal;
    }

    void checkLastValueComplex(const SignalPtr& signal, const double& realValue, const double& imaginaryValue)
    {
        ComplexNumberPtr ptr = signal.getLastValue().asPtr<IComplexNumber>();
        auto real = ptr.getReal();
        auto imaginary = ptr.getImaginary();

        ASSERT_DOUBLE_EQ(real, realValue);
        ASSERT_DOUBLE_EQ(imaginary, imaginaryValue);
    }

    void checkLastValueRange(const SignalPtr& signal, const int64_t& lowValue, const int64_t& highValue)
    {
        RangePtr ptr = signal.getLastValue().asPtr<IRange>();
        auto low = ptr.getLowValue().getIntValue();
        auto high = ptr.getHighValue().getIntValue();

        ASSERT_EQ(low, lowValue);
        ASSERT_EQ(high, highValue);
    }

    void checkLastValueListOfInt64(const SignalPtr& signal)
    {
        auto lv = signal.getLastValue();
        ListPtr<IInteger> ptr;
        ASSERT_NO_THROW(ptr = lv.asPtr<IList>());
        ASSERT_EQ(ptr.getItemAt(0), 4);
        ASSERT_EQ(ptr.getItemAt(1), 44);
    }

    template <typename T>
    void testGetLastValue(const SampleType& sampleType, const T& value)
    {
        auto daqServerSignal = Signal(NullContext(), nullptr, "Id");

        auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
        auto nodeId = serverSignal.registerOpcUaNode();

        daqServerSignal.setDescriptor(DataDescriptorBuilder().setSampleType(sampleType).build());

        auto dataPacket = DataPacket(daqServerSignal.getDescriptor(), 5);
        auto data = static_cast<T*>(dataPacket.getData());
        data[4] = value;

        daqServerSignal.sendPacket(dataPacket);

        auto clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

        if (sampleType == SampleType::Float32 || sampleType == SampleType::Float64)
        {
            ASSERT_DOUBLE_EQ(clientSignal.getLastValue(), static_cast<double>(value));
            ASSERT_DOUBLE_EQ(daqServerSignal.getLastValue(), static_cast<double>(value));
        }
        else
        {
            ASSERT_EQ(clientSignal.getLastValue(), static_cast<int64_t>(value));
            ASSERT_EQ(daqServerSignal.getLastValue(), static_cast<int64_t>(value));
        }
    }

    template <typename T>
    void testGetLastValueComplex(const SampleType& sampleType, const T& realValue, const T& imaginaryValue)
    {
        auto daqServerSignal = Signal(NullContext(), nullptr, "Id");

        auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
        auto nodeId = serverSignal.registerOpcUaNode();

        daqServerSignal.setDescriptor(DataDescriptorBuilder().setSampleType(sampleType).build());

        auto dataPacket = DataPacket(daqServerSignal.getDescriptor(), 5);
        auto data = static_cast<T*>(dataPacket.getData());
        data[8] = realValue;
        data[9] = imaginaryValue;

        daqServerSignal.sendPacket(dataPacket);

        auto clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

        checkLastValueComplex(clientSignal, realValue, imaginaryValue);
        checkLastValueComplex(daqServerSignal, realValue, imaginaryValue);
    }
};

TEST_F(TmsSignalTest, Create)
{
    SignalPtr signal = Signal(NullContext(), nullptr, "sig");
    auto tmsSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
}

TEST_F(TmsSignalTest, Register)
{
    SignalPtr signal = createSignal("Id");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);
    ASSERT_TRUE(clientSignal.assigned());
}

TEST_F(TmsSignalTest, AttrUniqueId)
{
    SignalPtr daqServerSignal = createSignal("Id");
    daqServerSignal.setActive(true);

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "Id", clientContext, nodeId);

    // UniqueId is set by core, so only test is that it is transferred correctly:
    ASSERT_EQ(daqServerSignal.getLocalId(), clientSignal.getLocalId());
}

TEST_F(TmsSignalTest, AttrActive)
{
    SignalPtr daqServerSignal = createSignal("Id");
    daqServerSignal.setActive(true);

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
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
    SignalPtr daqServerSignal = createSignal("Id");

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_TRUE(clientSignal.getPublic());

    // client side change is reflected on client side (public is not transferred):
    clientSignal.setPublic(false);
    ASSERT_FALSE(clientSignal.getPublic());

    clientSignal.setPublic(true);
    ASSERT_TRUE(clientSignal.getPublic());
}

TEST_F(TmsSignalTest, GetLastValueFloat32)
{
    float value = 4.1f;
    testGetLastValue(SampleType::Float32, value);
}

TEST_F(TmsSignalTest, GetLastValueFloat64)
{
    double value = 4.1;
    testGetLastValue(SampleType::Float64, value);
}

TEST_F(TmsSignalTest, GetLastValueInt8)
{
    int8_t value = 4;
    testGetLastValue(SampleType::Int8, value);
}

TEST_F(TmsSignalTest, GetLastValueInt16)
{
    int16_t value = 4;
    testGetLastValue(SampleType::Int16, value);
}

TEST_F(TmsSignalTest, GetLastValueInt32)
{
    int32_t value = 4;
    testGetLastValue(SampleType::Int32, value);
}
TEST_F(TmsSignalTest, GetLastValueInt64)
{
    int64_t value = 4;
    testGetLastValue(SampleType::Int64, value);
}

TEST_F(TmsSignalTest, GetLastValueUInt8)
{
    uint8_t value = 4u;
    testGetLastValue(SampleType::UInt8, value);
}

TEST_F(TmsSignalTest, GetLastValueUInt16)
{
    uint16_t value = 4u;
    testGetLastValue(SampleType::UInt16, value);
}

TEST_F(TmsSignalTest, GetLastValueUInt32)
{
    uint32_t value = 4u;
    testGetLastValue(SampleType::UInt32, value);
}
TEST_F(TmsSignalTest, GetLastValueUInt64)
{
    uint64_t value = 4u;
    testGetLastValue(SampleType::UInt64, value);
}

TEST_F(TmsSignalTest, GetLastValueRange)
{
    auto daqServerSignal = Signal(NullContext(), nullptr, "Id");

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    daqServerSignal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::RangeInt64).build());

    auto dataPacket = DataPacket(daqServerSignal.getDescriptor(), 5);
    auto data = static_cast<int64_t*>(dataPacket.getData());
    data[8] = 8;
    data[9] = 9;

    daqServerSignal.sendPacket(dataPacket);

    auto clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    checkLastValueRange(clientSignal, 8, 9);
    checkLastValueRange(daqServerSignal, 8, 9);
}

TEST_F(TmsSignalTest, GetLastValueListOfInt64)
{
    auto daqServerSignal = Signal(NullContext(), nullptr, "Id");

    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).setDimensions(dimensions).build();

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();
    daqServerSignal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::RangeInt64).build());

    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<int64_t*>(dataPacket.getData());
    data[8] = 4;
    data[9] = 44;

    daqServerSignal.sendPacket(dataPacket);

    auto clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    checkLastValueListOfInt64(clientSignal);
    checkLastValueListOfInt64(daqServerSignal);
}

TEST_F(TmsSignalTest, GetLastValueComplexFloat32)
{
    float real = 8.1f;
    float imaginary = 9.1f;
    testGetLastValueComplex(SampleType::ComplexFloat32, real, imaginary);
}

TEST_F(TmsSignalTest, GetLastValueComplexFloat64)
{
    double real = 8.1;
    double imaginary = 9.1;
    testGetLastValueComplex(SampleType::ComplexFloat64, real, imaginary);
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
                                    .setRule(ConstantDataRule())
                                    .setUnit(Unit("Symbol", 1, "Name", "Quantity"))  // Optional
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

    auto tmsServerSignal = TmsServerSignal(serverSignal, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_EQ(clientSignal.getActive(), serverSignal.getActive());
    // TODO: TMS signal should be implemented similar as fb, i.e. it needs to include property object
    // ASSERT_EQ(clientSignal.getName(), "My signal");
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
    ASSERT_EQ(clientRuleParameters.getCount(), 0u);

    auto clientUnit = clientDataDescriptor.getUnit();
    ASSERT_EQ(clientUnit.getQuantity(), "Quantity");
    ASSERT_EQ(clientUnit.getName(), "Name");
    ASSERT_EQ(clientUnit.getSymbol(), "Symbol");

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
    auto serverDomainSignal = TmsServerSignal(daqServerDomainSignal, this->getServer(), ctx, serverContext);
    auto domainNodeId = serverDomainSignal.registerOpcUaNode();

    SignalPtr daqServerSignal = createSignal("sig2");
    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    OpcUaNodeId referenceTypeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL);
    getServer()->addReference(nodeId, referenceTypeId, domainNodeId);
    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    [[maybe_unused]] SignalPtr clientDomainSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, domainNodeId);

    SignalPtr domainSignal;
    EXPECT_NO_THROW(domainSignal = clientSignal.getDomainSignal());
    ASSERT_EQ(clientDomainSignal, domainSignal);
}

TEST_F(TmsSignalTest, AttrRelatedSignals)
{
    SignalPtr daqServerSignal1 = createSignal("id1");
    auto serverSignal1 = TmsServerSignal(daqServerSignal1, this->getServer(), ctx, serverContext);
    auto nodeId1 = serverSignal1.registerOpcUaNode();

    SignalPtr daqServerSignal2 = createSignal("id2");
    auto serverSignal2 = TmsServerSignal(daqServerSignal2, this->getServer(), ctx, serverContext);
    auto nodeId2 = serverSignal2.registerOpcUaNode();

    SignalPtr daqServerSignal3 = createSignal("id3");
    auto serverSignal3 = TmsServerSignal(daqServerSignal3, this->getServer(), ctx, serverContext);
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
    SignalPtr daqServerSignal1 = createSignal("Id");
    auto serverSignal1 = TmsServerSignal(daqServerSignal1, this->getServer(), ctx, serverContext);
    auto nodeId1 = serverSignal1.registerOpcUaNode();

    SignalPtr clientSignal1 = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId1);
    ASSERT_EQ(clientSignal1.getConnections().getCount(), 0u);

    // TODO: Implement more test when related signals actually can be returned
    // ASSERT_EQ(relatedSignals1.getCount(), 2u);
    // ASSERT_EQ(relatedSignals2.getCount(), 2u);
    // ASSERT_EQ(relatedSignals3.getCount(), 2u);
}

TEST_F(TmsSignalTest, ComponentMethods)
{
    auto signal = createFullSignal("sig");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
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
    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_EQ(signal.getDescriptor().getRule(), clientSignal.getDescriptor().getRule());
}

TEST_F(TmsSignalTest, Visible)
{
    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(ExplicitDomainDataRule()).build();
    const auto signal = Signal(NullContext(), nullptr, "sig");
    signal.asPtr<IComponentPrivate>().unlockAllAttributes();
    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
    const auto nodeId = serverSignal.registerOpcUaNode();
    const SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_EQ(signal.getVisible(), clientSignal.getVisible());
    ASSERT_NO_THROW(clientSignal.setVisible(false));
    ASSERT_EQ(signal.getVisible(), false);
    ASSERT_EQ(clientSignal.getVisible(), false);

    signal.asPtr<IComponentPrivate>().lockAllAttributes();

    ASSERT_EQ(signal.getVisible(), clientSignal.getVisible());
    ASSERT_NO_THROW(clientSignal.setVisible(true));
    ASSERT_EQ(signal.getVisible(), false);
    ASSERT_EQ(clientSignal.getVisible(), false);
}

TEST_F(TmsSignalTest, GetNoValue)
{
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("domain stub")
                                .setSampleType(SampleType::UInt64)
                                .setOrigin("2024-01-08T00:02:03+00:00")
                                .setTickResolution(Ratio(1, 1000000))
                                .setRule(LinearDataRule(1000, 0))
                                .setUnit(Unit("s", -1, "seconds", "time"))
                                .build();
    const auto domainSignal = SignalWithDescriptor(NullContext(), domainDescriptor, nullptr, "domainSig");

    auto dataDescriptor = DataDescriptorBuilder().setName("stub").setSampleType(SampleType::Float64).build();
    const auto signal = SignalWithDescriptor(NullContext(), dataDescriptor, nullptr, "sig");

    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);
    ASSERT_TRUE(clientSignal.assigned());
    ASSERT_FALSE(clientSignal.getLastValue().assigned());
}

template <typename T>
static void sendValueToSignal(const SignalConfigPtr& signal, const T& value)
{
    if (!signal.assigned())
        return;

    DataPacketPtr packet;

    if (signal.getDomainSignal().assigned())
    {
        auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(), 1);
        packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), 1);
    }
    else
    {
        packet = DataPacket(signal.getDescriptor(), 1);
    }

    auto dst = static_cast<T*>(packet.getData());
    *dst = value;
    signal.sendPacket(packet);
}

TEST_F(TmsSignalTest, GetValue)
{
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("domain stub")
                                .setSampleType(SampleType::UInt64)
                                .setOrigin("2024-01-08T00:02:03+00:00")
                                .setTickResolution(Ratio(1, 1000000))
                                .setRule(LinearDataRule(1000, 0))
                                .setUnit(Unit("s", -1, "seconds", "time"))
                                .build();
    const auto domainSignal = SignalWithDescriptor(NullContext(), domainDescriptor, nullptr, "domainSig");

    auto dataDescriptor = DataDescriptorBuilder().setName("stub").setSampleType(SampleType::Float64).build();
    const auto signal = SignalWithDescriptor(NullContext(), dataDescriptor, nullptr, "sig");

    auto serverSignal = TmsServerSignal(signal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);
    ASSERT_TRUE(clientSignal.assigned());

    sendValueToSignal<double>(signal, 1.0);
    sendValueToSignal<double>(signal, 10.0);

    ASSERT_EQ(clientSignal.getLastValue(), 10.0);
}

TEST_F(TmsSignalTest, ValueAndAnalogValueDataTypeChange)
{
    // Create signal with Float64 descriptor
    auto descriptor1 = DataDescriptorBuilder()
                           .setSampleType(SampleType::Float64)
                           .setName("TestSignal")
                           .build();
    SignalConfigPtr serverSignal = Signal(ctx, nullptr, "sig");
    serverSignal.setDescriptor(descriptor1);
    serverSignal.setActive(true);

    // IMPORTANT: Keep TmsServerSignal alive for the duration of the test
    // It must be a shared_ptr so that weak_ptr in TmsServerContext can lock it
    auto tmsServerSignal = std::make_shared<TmsServerSignal>(serverSignal, this->getServer(), ctx, serverContext);
    auto signalNodeId = tmsServerSignal->registerOpcUaNode();

    // Get Value and AnalogValue node IDs using browser
    CachedReferenceBrowser browser(client);
    auto valueNodeId = browser.getChildNodeId(signalNodeId, "Value");
    auto analogValueNodeId = browser.getChildNodeId(signalNodeId, "AnalogValue");

    // Check initial data types are Float64
    auto valueDataType1 = client->readDataType(valueNodeId);
    ASSERT_EQ(valueDataType1, OpcUaNodeId(0, UA_NS0ID_DOUBLE));

    auto analogValueDataType1 = client->readDataType(analogValueNodeId);
    ASSERT_EQ(analogValueDataType1, OpcUaNodeId(0, UA_NS0ID_DOUBLE));

    // Change descriptor to Int32
    auto descriptor2 = DataDescriptorBuilder()
                           .setSampleType(SampleType::Int32)
                           .setName("TestSignal")
                           .build();
    serverSignal.setDescriptor(descriptor2);

    // Manually trigger the DataDescriptorChanged event on TmsServerSignal
    // Without Instance, core events are not generated automatically
    auto eventArgs = CoreEventArgsDataDescriptorChanged(descriptor2);
    tmsServerSignal->onCoreEvent(eventArgs);

    // Create a new browser to get fresh references after node recreation
    // Old browser may have cached references to deleted nodes
    CachedReferenceBrowser newBrowser(client);
    
    // Get new node IDs after recreation (old ones may be invalid)
    auto newValueNodeId = newBrowser.getChildNodeId(signalNodeId, "Value");
    auto newAnalogValueNodeId = newBrowser.getChildNodeId(signalNodeId, "AnalogValue");
    
    // Verify that nodes exist
    ASSERT_TRUE(client->nodeExists(newValueNodeId)) 
        << "New Value node should exist after recreation";
    ASSERT_TRUE(client->nodeExists(newAnalogValueNodeId))
        << "New AnalogValue node should exist after recreation";

    // Check that data types have changed to Int32
    auto valueDataType2 = client->readDataType(newValueNodeId);
    ASSERT_EQ(valueDataType2, OpcUaNodeId(0, UA_NS0ID_INT32)) 
        << "Value node data type should be Int32 after descriptor change. Got: " << valueDataType2.toString();

    auto analogValueDataType2 = client->readDataType(newAnalogValueNodeId);
    ASSERT_EQ(analogValueDataType2, OpcUaNodeId(0, UA_NS0ID_INT32))
        << "AnalogValue node data type should be Int32 after descriptor change. Got: " << analogValueDataType2.toString();
}
