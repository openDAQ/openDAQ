#include <coreobjects/property_object_class_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/removable_ptr.h>
#include <opendaq/signal_events.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/tags_factory.h>
#include <opendaq/input_port_notifications.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/scheduler_factory.h>
#include <thread>

using SignalTest = testing::Test;

using namespace daq;

class ConnectionMockImpl : public ImplementationOf<IConnection>
{
public:
    bool packetEnqueued{ false };
    int packetsEnqueued{0};

    ErrCode INTERFACE_FUNC enqueue(IPacket* packet) override
    {
        packetEnqueued = true;
        packetsEnqueued++;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enqueueMultiple(IList* packets) override
    {
        packetEnqueued = true;
        packetsEnqueued += ListPtr<IPacket>::Borrow(packets).getCount();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enqueueAndStealRef(IPacket* packet) override
    {
        packetsEnqueued++;
        packetEnqueued = true;

        if (packet != nullptr)
            packet->releaseRef();

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enqueueMultipleAndStealRef(IList* packets) override
    {
        packetsEnqueued += ListPtr<IPacket>::Borrow(packets).getCount();
        packetEnqueued = true;

        if (packets != nullptr)
            packets->releaseRef();

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enqueueOnThisThread(IPacket* packet) override
    {
        packetEnqueued = true;
        packetsEnqueued++;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC dequeue(IPacket** packet) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC dequeueAll(IList** packets) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC peek(IPacket** packet) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getPacketCount(SizeT* packetCount) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getInputPort(IInputPort** inputPort) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getAvailableSamples(SizeT* samples) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSamplesUntilNextDescriptor(SizeT* samples) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSamplesUntilNextEventPacket(SizeT* samples) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSamplesUntilNextGapPacket(SizeT* samples) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC hasEventPacket(Bool* hasEventPacket) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC hasGapPacket(Bool* hasGapPacket) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC isRemote(Bool* remote) override
    {
        *remote = False;
        return OPENDAQ_SUCCESS;
    }
};

class PacketMockImpl : public ImplementationOf<IPacket>
{
public:
    ErrCode INTERFACE_FUNC getType(PacketType* type) override
    {
        *type = PacketType::Data;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC subscribeForDestructNotification(IPacketDestructCallback* packetDestructCallback) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }

    ErrCode INTERFACE_FUNC getRefCount(SizeT* refCount) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
};

ConnectionPtr ConnectionMock()
{
    IConnection* intf;
    checkErrorInfo(createObject<IConnection, ConnectionMockImpl>(&intf));
    return intf;
}

PacketPtr PacketMock()
{
    IPacket* intf;
    checkErrorInfo(createObject<IPacket, PacketMockImpl>(&intf));
    return intf;
}

class DataDescriptorMockImpl : public ImplementationOf<IDataDescriptor>
{
public:
    ErrCode INTERFACE_FUNC getName(IString** name) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getDimensions(IList** dimensions) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSampleType(SampleType* sampleType) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override
    {
        return OPENDAQ_SUCCESS;
    }

    virtual ErrCode INTERFACE_FUNC getValueRange(IRange** range) override
    {
        return OPENDAQ_SUCCESS;
    }

    virtual ErrCode INTERFACE_FUNC getRule(IDataRule** rule) override
    {
        return OPENDAQ_SUCCESS;
    }

    virtual ErrCode INTERFACE_FUNC getOrigin(IString** origin) override
    {
        return OPENDAQ_SUCCESS;
    }

    virtual ErrCode INTERFACE_FUNC getTickResolution(IRatio** tickResolution) override
    {
        return OPENDAQ_SUCCESS;
    }

    virtual ErrCode INTERFACE_FUNC getPostScaling(IScaling** scaling) override
    {
        return OPENDAQ_SUCCESS;
    }

    virtual ErrCode INTERFACE_FUNC getStructFields(IList** structFields) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSampleSize(SizeT* size) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getRawSampleSize(SizeT* rawSampleSize) override
    {
        return OPENDAQ_SUCCESS;
    }
};

TEST_F(SignalTest, IsComponent)
{
    auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_NO_THROW(signal.asPtr<IComponent>());
}

TEST_F(SignalTest, SignalConnections)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto connections = signal.getConnections();
    ASSERT_EQ(connections.getCount(), 0u);

    auto conn1 = ConnectionMock();
    checkErrorInfo(signal.asPtr<ISignalEvents>()->listenerConnected(conn1));
    connections = signal.getConnections();
    ASSERT_EQ(connections.getCount(), 1u);

    auto conn2 = ConnectionMock();
    checkErrorInfo(signal.asPtr<ISignalEvents>()->listenerConnected(conn2));
    connections = signal.getConnections();
    ASSERT_EQ(connections.getCount(), 2u);

    ASSERT_THROW(checkErrorInfo(signal.asPtr<ISignalEvents>()->listenerConnected(conn2)), DuplicateItemException);

    checkErrorInfo(signal.asPtr<ISignalEvents>()->listenerDisconnected(conn2));
    connections = signal.getConnections();
    ASSERT_EQ(connections.getCount(), 1u);

    ASSERT_THROW(checkErrorInfo(signal.asPtr<ISignalEvents>()->listenerDisconnected(conn2)), NotFoundException);
}

TEST_F(SignalTest, RelatedSignals)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto relatedSignals = signal.getRelatedSignals();
    ASSERT_EQ(relatedSignals.getCount(), 0u);

    auto signal1 = Signal(NullContext(), nullptr, "sig");
    signal.addRelatedSignal(signal1);
    relatedSignals = signal.getRelatedSignals();
    ASSERT_EQ(relatedSignals.getCount(), 1u);
    ASSERT_EQ(relatedSignals[0], signal1);

    auto signal2 = Signal(NullContext(), nullptr, "sig");
    signal.addRelatedSignal(signal2);
    relatedSignals = signal.getRelatedSignals();
    ASSERT_EQ(relatedSignals.getCount(), 2u);

    ASSERT_THROW(signal.addRelatedSignal(signal2), DuplicateItemException);

    signal.removeRelatedSignal(signal2);
    relatedSignals = signal.getRelatedSignals();
    ASSERT_EQ(relatedSignals.getCount(), 1u);

    ASSERT_THROW(signal.removeRelatedSignal(signal2), NotFoundException);

    signal.clearRelatedSignals();
    relatedSignals = signal.getRelatedSignals();
    ASSERT_EQ(relatedSignals.getCount(), 0u);
}

TEST_F(SignalTest, EmptyClassName)
{
    auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_EQ(signal.getClassName(), "");
}

// TODO: reintroduce className parameter when duck-typing is limited.

/*
TEST_F(SignalTest, ClassName)
{
    const auto context = Context(nullptr, Logger(), TypeManager(), nullptr);
    auto rangeItemClass = PropertyObjectClass("TestClass");
    context.getTypeManager().addType(rangeItemClass);

    auto signal = Signal(context, nullptr, "sig", "TestClass");
    ASSERT_EQ(signal.getClassName(), "TestClass");
}
*/

TEST_F(SignalTest, DomainSignal)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_FALSE(signal.getDomainSignal().assigned());

    auto signal1 = Signal(NullContext(), nullptr, "sig");
    signal.setDomainSignal(signal1);
    ASSERT_EQ(signal.getDomainSignal(), signal1);

    signal.setDomainSignal(nullptr);
    ASSERT_FALSE(signal.getDomainSignal().assigned());
}

TEST_F(SignalTest, Active)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_TRUE(signal.getActive());

    signal.setActive(False);
    ASSERT_FALSE(signal.getActive());
}

TEST_F(SignalTest, Public)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_TRUE(signal.getPublic());

    signal.setPublic(False);
    ASSERT_FALSE(signal.getPublic());
}

TEST_F(SignalTest, Name)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_EQ(signal.getName(), "sig");

    signal.setName("Signal");
    ASSERT_EQ(signal.getName(), "Signal");
}

TEST_F(SignalTest, Descriptor)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_EQ(signal.getDescription(), "");

    signal.setDescription("Description");
    ASSERT_EQ(signal.getDescription(), "Description");
}

TEST_F(SignalTest, SignalDescriptor)
{
    auto dataDescriptorImpl = new DataDescriptorMockImpl();
    ObjectPtr<IDataDescriptor> dataDescriptor;
    checkErrorInfo(dataDescriptorImpl->queryInterface(IDataDescriptor::Id, reinterpret_cast<void**>(&dataDescriptor)));

    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_FALSE(signal.getDescriptor().assigned());

    signal.setDescriptor(dataDescriptor);
    ASSERT_EQ(signal.getDescriptor(), dataDescriptor);
}

TEST_F(SignalTest, SignalDescriptorStruct)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    const auto descriptor = DataDescriptorBuilder()
                                .setName("MyTestStructType")
                                .setSampleType(SampleType::Struct)
                                .setStructFields(List<DataDescriptorPtr>(
                                    DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build(),
                                    DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build(),
                                    DataDescriptorBuilder()
                                        .setName("Special1")
                                        .setSampleType(SampleType::Struct)
                                        .setStructFields(List<DataDescriptorPtr>(
                                            DataDescriptorBuilder().setName("NestedInt64").setSampleType(SampleType::Int64).build()))
                                        .build()))
                                .build();

    signal.setDescriptor(descriptor);

    ASSERT_EQ(signal.getDescriptor(), descriptor);

    auto fieldNames = List<IString>();
    auto fieldTypes = List<IType>();

    fieldNames.pushBack("Int32");
    fieldNames.pushBack("Float64");

    fieldTypes.pushBack(SimpleType(CoreType::ctInt));
    fieldTypes.pushBack(SimpleType(CoreType::ctFloat));

    auto nestedFieldNames = List<IString>();
    auto nestedFieldTypes = List<IType>();

    nestedFieldNames.pushBack("NestedInt64");
    nestedFieldTypes.pushBack(SimpleType(CoreType::ctInt));

    auto nestedStruct = StructType("Special1", nestedFieldNames, nestedFieldTypes);

    fieldNames.pushBack("Special1");
    fieldTypes.pushBack(nestedStruct);

    const auto type = signal.getContext().getTypeManager().getType("MyTestStructType");
    const auto realType = StructType("MyTestStructType", fieldNames, fieldTypes);

    ASSERT_EQ(type, realType);
}

TEST_F(SignalTest, SignalDescriptorStructSameTwice)
{
    const auto contex = NullContext();

    const auto signal1 = Signal(contex, nullptr, "sig1");
    const auto signal2 = Signal(contex, nullptr, "sig2");

    const auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build()))
            .build();

    signal1.setDescriptor(descriptor);
    signal2.setDescriptor(descriptor);

    ASSERT_EQ(signal1.getDescriptor(), signal2.getDescriptor());
}

TEST_F(SignalTest, SignalDescriptorStructSameNameDifferentDescriptor)
{
    const auto contex = NullContext();

    const auto signal1 = Signal(contex, nullptr, "sig1");
    const auto signal2 = Signal(contex, nullptr, "sig2");

    const auto descriptor1 =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build()))
            .build();

    const auto descriptor2 =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build()))
            .build();

    signal1.setDescriptor(descriptor1);
    signal2.setDescriptor(descriptor2);

    ASSERT_EQ(signal1.getDescriptor(), descriptor1);
    ASSERT_EQ(signal2.getDescriptor(), descriptor2);

    auto dataPacket1 = DataPacket(descriptor1, 5);
    auto data1 = static_cast<int32_t*>(dataPacket1.getData());
    data1[4] = 4;
    signal1.sendPacket(dataPacket1);

    auto dataPacket2 = DataPacket(descriptor2, 5);
    auto data2 = static_cast<double*>(dataPacket2.getData());
    data2[4] = 4.2;
    signal2.sendPacket(dataPacket2);

    const auto lv1 = signal1.getLastValue();
    StructPtr sp1;
    ASSERT_NO_THROW(sp1 = lv1.asPtr<IStruct>());
    ASSERT_EQ(sp1.get("Int32"), 4);

    // Throws because descriptor2 has the same name as descriptor1 (but is different)
    // and hence the the struct type can't be added to the type manager and hence
    // later can't be found in the type manager
    ASSERT_THROW(signal2.getLastValue(), NotFoundException);
}

TEST_F(SignalTest, SendNullPacket)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_THROW(signal.sendPacket(nullptr), ArgumentNullException);
}

TEST_F(SignalTest, SendPacket)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    auto connImpl = new ConnectionMockImpl();
    ConnectionPtr conn;
    checkErrorInfo(connImpl->queryInterface(IConnection::Id, reinterpret_cast<void**>(&conn)));

    signal.asPtr<ISignalEvents>()->listenerConnected(conn);

    auto packet = PacketMock();
    signal.sendPacket(packet);

    ASSERT_TRUE(connImpl->packetEnqueued);
}

TEST_F(SignalTest, SendPackets)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    auto connImpl = new ConnectionMockImpl();
    ConnectionPtr conn;
    checkErrorInfo(connImpl->queryInterface(IConnection::Id, reinterpret_cast<void**>(&conn)));

    signal.asPtr<ISignalEvents>()->listenerConnected(conn);

    auto packet0 = PacketMock();
    auto packet1 = PacketMock();

    signal.sendPackets(List<IPacket>(packet0, packet1));
    ASSERT_EQ(connImpl->packetsEnqueued, 3);

    ASSERT_TRUE(connImpl->packetEnqueued);
}

TEST_F(SignalTest, SendAndReleasePacket)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    signal.setPublic(False);

    auto connImpl = new ConnectionMockImpl();
    ConnectionPtr conn;
    checkErrorInfo(connImpl->queryInterface(IConnection::Id, reinterpret_cast<void**>(&conn)));

    signal.asPtr<ISignalEvents>()->listenerConnected(conn);

    auto packet = PacketMock();
    signal.sendPacket(std::move(packet));

    ASSERT_TRUE(connImpl->packetEnqueued);
}

TEST_F(SignalTest, SendAndReleasePackets)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    signal.setPublic(False);

    auto connImpl = new ConnectionMockImpl();
    ConnectionPtr conn;
    checkErrorInfo(connImpl->queryInterface(IConnection::Id, reinterpret_cast<void**>(&conn)));

    signal.asPtr<ISignalEvents>()->listenerConnected(conn);

    auto packets = List<IPacket>(PacketMock(), PacketMock());

    signal.sendPackets(std::move(packets));

    ASSERT_EQ(connImpl->packetsEnqueued, 3);
    ASSERT_TRUE(connImpl->packetEnqueued);
}

TEST_F(SignalTest, SetDescriptorWithConnection)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    auto connImpl = new ConnectionMockImpl();
    ConnectionPtr conn;
    checkErrorInfo(connImpl->queryInterface(IConnection::Id, reinterpret_cast<void**>(&conn)));

    signal.asPtr<ISignalEvents>()->listenerConnected(conn);

    auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    signal.setDescriptor(desc);

    ASSERT_TRUE(connImpl->packetEnqueued);
}

TEST_F(SignalTest, SetDomainDescriptorWithConnection)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    const auto domainSignal = Signal(NullContext(), nullptr, "sig");
    signal.setDomainSignal(domainSignal);

    auto connImpl = new ConnectionMockImpl();
    ConnectionPtr conn;
    checkErrorInfo(connImpl->queryInterface(IConnection::Id, reinterpret_cast<void**>(&conn)));

    signal.asPtr<ISignalEvents>()->listenerConnected(conn);

    auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    domainSignal.setDescriptor(desc);

    ASSERT_TRUE(connImpl->packetEnqueued);
}

TEST_F(SignalTest, SetSharedDomainDescriptor)
{
    auto signal1 = Signal(NullContext(), nullptr, "sig");
    {
        auto signal2 = Signal(NullContext(), nullptr, "sig");
        {
            auto signal3 = Signal(NullContext(), nullptr, "sig");
            auto domainSignal = Signal(NullContext(), nullptr, "sig");

            signal3.setDomainSignal(domainSignal);
            signal2.setDomainSignal(domainSignal);
            signal1.setDomainSignal(domainSignal);
        }
    }
}

TEST_F(SignalTest, Remove)
{
    auto signal = Signal(NullContext(), nullptr, "sig");

    auto removable = signal.asPtr<IRemovable>(true);
    ASSERT_FALSE(removable.isRemoved());

    removable.remove();

    ASSERT_TRUE(removable.isRemoved());
}

TEST_F(SignalTest, Streamed)
{
    auto signal = Signal(NullContext(), nullptr, "sig");

    ASSERT_FALSE(signal.getStreamed());

    ASSERT_EQ(signal->setStreamed(false), OPENDAQ_IGNORED);
    ASSERT_EQ(signal->setStreamed(true), OPENDAQ_IGNORED);

    ASSERT_FALSE(signal.getStreamed());
}

TEST_F(SignalTest, StandardProperties)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto signal = Signal(NullContext(), nullptr, "sig");

    signal.setName(name);
    signal.setDescription(desc);

    ASSERT_EQ(signal.getName(), name);
    ASSERT_EQ(signal.getDescription(), desc);
}

TEST_F(SignalTest, SerializeAndUpdate)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto signal = Signal(NullContext(), nullptr, "sig");

    signal.setName(name);
    signal.setDescription(desc);

    const auto serializer = JsonSerializer(True);
    signal.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto newSignal = Signal(NullContext(), nullptr, "sig");
    const auto deserializer = JsonDeserializer();
    const auto updatable = newSignal.asPtr<IUpdatable>();

    deserializer.update(updatable, str1);

    ASSERT_EQ(newSignal.getName(), name);
    ASSERT_EQ(newSignal.getDescription(), desc);

    const auto serializer2 = JsonSerializer(True);
    newSignal.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(SignalTest, SerializeAndDeserialize)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    const auto domainSignal = Signal(NullContext(), nullptr, "domainSig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    signal.setName("sig_name");
    signal.setDescription("sig_description");
    signal.setActive(false);
    signal.setDescriptor(descriptor);

    domainSignal.setName("domainSig_name");
    domainSignal.setDescription("domainSig_description");

    signal.setDomainSignal(domainSignal);

    const auto serializer = JsonSerializer(True);
    signal.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "sig");

    const SignalConfigPtr newSignal = deserializer.deserialize(str1, deserializeContext, nullptr);

    const auto deserializedDomainSignalId = newSignal.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");
    ASSERT_EQ(deserializedDomainSignalId, domainSignal.getGlobalId());
    newSignal.setDomainSignal(domainSignal);

    ASSERT_EQ(newSignal.getName(), signal.getName());
    ASSERT_EQ(newSignal.getDescription(), signal.getDescription());
    ASSERT_EQ(newSignal.getActive(), signal.getActive());
    ASSERT_EQ(newSignal.getDescriptor(), signal.getDescriptor());

    const auto serializer2 = JsonSerializer(True);
    newSignal.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(SignalTest, LockedAttributes)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_EQ(signal.getPublic(), true);

    ASSERT_NO_THROW(signal.setPublic(false));
    ASSERT_EQ(signal.getPublic(), false);

    signal.asPtr<IComponentPrivate>().lockAllAttributes();

    ASSERT_NO_THROW(signal.setPublic(false));
    ASSERT_EQ(signal.getPublic(), false);
}

TEST_F(SignalTest, NoLastValue)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_FALSE(signal.getLastValue().assigned());
}

TEST_F(SignalTest, GetLastValue)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    auto dataPacket = DataPacket(descriptor, 5);
    int64_t* data = static_cast<int64_t*>(dataPacket.getData());
    data[4] = 4;

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    IntegerPtr integerPtr;
    ASSERT_NO_THROW(integerPtr = lastValuePacket.asPtr<IInteger>());
    ASSERT_EQ(integerPtr, 4);
}

TEST_F(SignalTest, GetLastValueAfterMultipleSend)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    {
        auto dataPacket = DataPacket(descriptor, 1);
        int64_t* data = static_cast<int64_t*>(dataPacket.getData());
        data[0] = 0;
        signal.sendPacket(dataPacket);
    }

    {
        auto dataPacket = DataPacket(descriptor, 1);
        int64_t* data = static_cast<int64_t*>(dataPacket.getData());
        data[0] = 1;
        signal.sendPacket(dataPacket);
    }

    auto lastValuePacket = signal.getLastValue();
    IntegerPtr integerPtr;
    ASSERT_NO_THROW(integerPtr = lastValuePacket.asPtr<IInteger>());
    ASSERT_EQ(integerPtr, 1);
}

TEST_F(SignalTest, GetLastValueAfterEmptyPacket)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    {
        auto dataPacket = DataPacket(descriptor, 1);
        int64_t* data = static_cast<int64_t*>(dataPacket.getData());
        data[0] = 0;
        signal.sendPacket(dataPacket);
    }

    {
        auto dataPacket = DataPacket(descriptor, 0);
        signal.sendPacket(dataPacket);
    }

    auto lastValuePacket = signal.getLastValue();
    IntegerPtr integerPtr;
    ASSERT_NO_THROW(integerPtr = lastValuePacket.asPtr<IInteger>());
    ASSERT_EQ(integerPtr, 0);
}

TEST_F(SignalTest, GetLastValueDisabled)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    const auto privateSignal = signal.asPtrOrNull<ISignalPrivate>();
    ASSERT_TRUE(privateSignal.assigned());
    privateSignal.enableKeepLastValue(false);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    {
        auto dataPacket = DataPacket(descriptor, 1);
        int64_t* data = static_cast<int64_t*>(dataPacket.getData());
        data[0] = 1;
        signal.sendPacket(dataPacket);
    }

    ASSERT_FALSE(signal.getLastValue().assigned());
}

TEST_F(SignalTest, GetLastValueNonPublicDisabled)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    signal.setPublic(False);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    {
        auto dataPacket = DataPacket(descriptor, 1);
        int64_t* data = static_cast<int64_t*>(dataPacket.getData());
        data[0] = 1;
        signal.sendPacket(dataPacket);
    }

    ASSERT_FALSE(signal.getLastValue().assigned());
}

TEST_F(SignalTest, GetLastValueInvisibleDisabled)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    signal.template asPtr<IComponentPrivate>().unlockAttributes(List<IString>("Visible"));
    signal.setVisible(False);
    signal.template asPtr<IComponentPrivate>().lockAttributes(List<IString>("Visible"));

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    {
        auto dataPacket = DataPacket(descriptor, 1);
        int64_t* data = static_cast<int64_t*>(dataPacket.getData());
        data[0] = 1;
        signal.sendPacket(dataPacket);
    }

    ASSERT_FALSE(signal.getLastValue().assigned());
}

class ListenerImpl : public ImplementationOfWeak<IInputPortNotifications>
{
public:
    ListenerImpl(const std::function<void(const InputPortPtr& inputPort)>& onPacketRecieved)
        : onPacketRecieved(onPacketRecieved)
    {
    }

    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override
    {
        *accept = True;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC connected(IInputPort* port) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override
    {
        onPacketRecieved(port);
        return OPENDAQ_SUCCESS;
    }

private:
    std::function<void(const InputPortPtr& inputPort)> onPacketRecieved;
};

TEST_F(SignalTest, LastReferenceSameThread)
{
    const auto logger = Logger();
    const auto ctx = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(False);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    int dataPacketsRecieved = 0;
    const auto listener = createWithImplementation<IInputPortNotifications, ListenerImpl>(
        [&dataPacketsRecieved](const InputPortPtr& port)
        {
            const auto conn = port.getConnection();
            if (!conn.assigned())
                return;

            auto packet = conn.dequeue();
            while (packet.assigned())
            {
                if (packet.getType() == PacketType::Data)
                {
                    dataPacketsRecieved++;
                    ASSERT_EQ(packet.getRefCount(), 1u);
                }
                packet = conn.dequeue();
            }
        });

    const auto ip = InputPort(ctx, nullptr, "ip");
    ip.setNotificationMethod(PacketReadyNotification::SameThread);
    ip.setListener(listener);

    ip.connect(signal);

    auto dataPacket = DataPacket(descriptor, 1);
    int64_t* data = static_cast<int64_t*>(dataPacket.getData());
    data[0] = 1;

    auto packet = std::move(dataPacket);
    signal.sendPacket(std::move(packet));

    ASSERT_EQ(dataPacketsRecieved, 1);
}

TEST_F(SignalTest, LastReferenceScheduler)
{
    std::mutex mtx;
    std::condition_variable cv;
    bool packetRead = false;
    std::thread::id threadId;

    const auto logger = Logger();
    const auto scheduler = Scheduler(logger);
    const auto ctx = Context(scheduler, logger, TypeManager(), nullptr, nullptr);

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(False);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    int dataPacketsRecieved = 0;
    const auto listener = createWithImplementation<IInputPortNotifications, ListenerImpl>(
        [&dataPacketsRecieved, &mtx, &cv, &packetRead, &threadId](const InputPortPtr& port)
        {
            const auto conn = port.getConnection();
            if (!conn.assigned())
                return;

            auto packet = conn.dequeue();
            while (packet.assigned())
            {
                if (packet.getType() == PacketType::Data)
                {
                    dataPacketsRecieved++;
                    threadId = std::this_thread::get_id();
                    ASSERT_EQ(packet.getRefCount(), 1u);

                    std::unique_lock lock(mtx);
                    packetRead = true;
                    cv.notify_one();
                }
                packet = conn.dequeue();
            }
        });

    const auto ip = InputPort(ctx, nullptr, "ip");
    ip.setNotificationMethod(PacketReadyNotification::Scheduler);
    ip.setListener(listener);

    ip.connect(signal);

    auto dataPacket = DataPacket(descriptor, 1);
    int64_t* data = static_cast<int64_t*>(dataPacket.getData());
    data[0] = 1;

    auto packet = std::move(dataPacket);
    signal.sendPacket(std::move(packet));

    {
        std::unique_lock lock(mtx);
        while (!packetRead)
            cv.wait(lock);
    }

    ASSERT_EQ(dataPacketsRecieved, 1);
    ASSERT_NE(std::this_thread::get_id(), threadId);

    scheduler.stop();
}

TEST_F(SignalTest, LastReferenceSameThreadMultiPackets)
{
    const auto logger = Logger();
    const auto ctx = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(False);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    int dataPacketsRecieved = 0;
    const auto listener = createWithImplementation<IInputPortNotifications, ListenerImpl>(
        [&dataPacketsRecieved](const InputPortPtr& port)
        {
            const auto conn = port.getConnection();
            if (!conn.assigned())
                return;

            auto packet = conn.dequeue();
            while (packet.assigned())
            {
                if (packet.getType() == PacketType::Data)
                {
                    dataPacketsRecieved++;
                    ASSERT_EQ(packet.getRefCount(), 1u);
                }
                packet = conn.dequeue();
            }
        });

    const auto ip = InputPort(ctx, nullptr, "ip");
    ip.setNotificationMethod(PacketReadyNotification::SameThread);
    ip.setListener(listener);

    ip.connect(signal);

    auto packets = List<IPacket>(DataPacket(descriptor, 1), DataPacket(descriptor, 1));

    signal.sendPackets(std::move(packets));

    ASSERT_EQ(dataPacketsRecieved, 2);
}


TEST_F(SignalTest, GetLastValueRange)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::RangeInt64).build();

    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<int64_t*>(dataPacket.getData());
    data[8] = 8;
    data[9] = 9;

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    RangePtr rangePtr;
    ASSERT_NO_THROW(rangePtr = lastValuePacket.asPtr<IRange>());
    ASSERT_EQ(rangePtr.getLowValue(), 8);
    ASSERT_EQ(rangePtr.getHighValue(), 9);
}

TEST_F(SignalTest, GetLastValueComplexFloat32)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::ComplexFloat32).build();

    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<float*>(dataPacket.getData());
    data[8] = 8.1f;
    data[9] = 9.1f;

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    ComplexNumberPtr complexPtr;
    ASSERT_NO_THROW(complexPtr = lastValuePacket.asPtr<IComplexNumber>());
    ASSERT_FLOAT_EQ(complexPtr.getReal(), 8.1f);
    ASSERT_FLOAT_EQ(complexPtr.getImaginary(), 9.1f);
}

TEST_F(SignalTest, GetLastValueComplexFloat64)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::ComplexFloat64).build();

    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<double*>(dataPacket.getData());
    data[8] = 8.1;
    data[9] = 9.1;

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    ComplexNumberPtr complexPtr;
    ASSERT_NO_THROW(complexPtr = lastValuePacket.asPtr<IComplexNumber>());
    ASSERT_DOUBLE_EQ(complexPtr.getReal(), 8.1);
    ASSERT_DOUBLE_EQ(complexPtr.getImaginary(), 9.1);
}

TEST_F(SignalTest, TestSignalActiveSendPacket)
{
    const auto context = NullContext();
    const auto signal = Signal(context, nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Float64).build();
    const auto ip = InputPort(context, nullptr, "ip");
    ip.connect(signal);

    auto dataPacket = DataPacket(descriptor, 1);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2u);

    signal.setActive(false);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2u);

    auto descriptor1 = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int16).build();
    signal.setDescriptor(descriptor1);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 3u);
}

TEST_F(SignalTest, TestInputPortActiveSendPacket)
{
    const auto context = NullContext();
    const auto signal = Signal(context, nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Float64).build();
    const auto ip = InputPort(context, nullptr, "ip");
    ip.connect(signal);

    PacketPtr dataPacket = DataPacket(descriptor, 1);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2u);

    ip.setActive(false);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2u);

    auto descriptor1 = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int16).build();
    signal.setDescriptor(descriptor1);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 3u);
}

TEST_F(SignalTest, GetLastValueStruct)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    const auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build(),
                                                     DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build()))
            .build();

    // Set the descriptor, thereby adding the struct type to the type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    auto sizeInBytes = sizeof(int32_t) + sizeof(double);
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is double
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lastValuePacket = signal.getLastValue();

    // Create data structure
    StructPtr structPtr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(structPtr = lastValuePacket.asPtr<IStruct>());

    // Check first member
    ASSERT_EQ(structPtr.get("Int32"), 12);

    // Check second member
    ASSERT_DOUBLE_EQ(structPtr.get("Float64"), 15.1);
}

TEST_F(SignalTest, GetLastValueLinearDataRule)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setRule(LinearDataRule(3, 2)).setName("test").setSampleType(SampleType::Float64).build();

    auto dataPacket = DataPacket(descriptor, 5, 7);

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    FloatPtr ptr;
    ASSERT_NO_THROW(ptr = lastValuePacket.asPtr<IFloat>());
    ASSERT_EQ(ptr, 21.0);
}

TEST_F(SignalTest, GetLastValueLinearScaling)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder()
                          .setPostScaling(LinearScaling(3, 7, SampleType::Int32, ScaledSampleType::Float64))
                          .setName("test")
                          .setSampleType(SampleType::Float64)
                          .build();

    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<int32_t*>(dataPacket.getRawData());
    data[4] = 4;

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    FloatPtr ptr;
    ASSERT_NO_THROW(ptr = lastValuePacket.asPtr<IFloat>());
    ASSERT_EQ(ptr, 19.0);
}

TEST_F(SignalTest, GetLastValueListOfInt)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).setDimensions(dimensions).build();

    auto dataPacket = DataPacket(descriptor, 5);
    int64_t* data = static_cast<int64_t*>(dataPacket.getData());
    data[8] = 4;
    data[9] = 44;

    signal.sendPacket(dataPacket);

    auto lv = signal.getLastValue();
    ListPtr<IInteger> ptr;
    ASSERT_NO_THROW(ptr = lv.asPtr<IList>());
    ASSERT_EQ(ptr.getItemAt(0), 4);
    ASSERT_EQ(ptr.getItemAt(1), 44);
}

TEST_F(SignalTest, GetLastValueListOfStructs)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build(),
                                                     DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build()))
            .setDimensions(dimensions)
            .build();

    // Set the descriptor, thereby adding the struct type to the type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    auto sizeInBytes = 2 * (sizeof(int32_t) + sizeof(double));
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is double
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;

    // Third member of data is int32_t
    void* c = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double);
    auto C = static_cast<int32_t*>(c);
    *C = 13;

    // Fourth member of data is double
    void* d = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double) + sizeof(int32_t);
    auto D = static_cast<double*>(d);
    *D = 16.1;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lv = signal.getLastValue();

    // Create data structure
    ListPtr<IStruct> ptr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(ptr = lv.asPtr<IList>());

    // Check all inner members of the list
    const auto first = ptr.getItemAt(0).get("Int32");
    const auto second = ptr.getItemAt(0).get("Float64");
    const auto third = ptr.getItemAt(1).get("Int32");
    const auto fourth = ptr.getItemAt(1).get("Float64");
    ASSERT_EQ(first, 12);
    ASSERT_DOUBLE_EQ(second, 15.1);
    ASSERT_EQ(third, 13);
    ASSERT_DOUBLE_EQ(fourth, 16.1);
}

TEST_F(SignalTest, GetLastValueListOfStructsNested)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(
                DataDescriptorBuilder()
                    .setName("Struct1")
                    .setSampleType(SampleType::Struct)
                    .setStructFields(
                        List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build()))
                    .build(),
                DataDescriptorBuilder()
                    .setName("Struct2")
                    .setSampleType(SampleType::Struct)
                    .setStructFields(
                        List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build()))
                    .build()))
            .setDimensions(dimensions)
            .build();

    // Set the descriptor, thereby adding the struct type to the type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    auto sizeInBytes = 2 * (sizeof(int32_t) + sizeof(double));
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is double
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;

    // Third member of data is int32_t
    void* c = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double);
    auto C = static_cast<int32_t*>(c);
    *C = 13;

    // Fourth member of data is double
    void* d = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double) + sizeof(int32_t);
    auto D = static_cast<double*>(d);
    *D = 16.1;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lv = signal.getLastValue();

    // Create data structure
    ListPtr<IStruct> ptr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(ptr = lv.asPtr<IList>());

    // Check all inner members of the list
    const auto first = static_cast<StructPtr>(ptr.getItemAt(0).get("Struct1")).get("Int32");
    const auto second = static_cast<StructPtr>(ptr.getItemAt(0).get("Struct2")).get("Float64");
    const auto third = static_cast<StructPtr>(ptr.getItemAt(1).get("Struct1")).get("Int32");
    const auto fourth = static_cast<StructPtr>(ptr.getItemAt(1).get("Struct2")).get("Float64");
    ASSERT_EQ(first, 12);
    ASSERT_DOUBLE_EQ(second, 15.1);
    ASSERT_EQ(third, 13);
    ASSERT_DOUBLE_EQ(fourth, 16.1);
}

TEST_F(SignalTest, GetLastValueListOfStructsOfListOfInt32)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(
                DataDescriptorBuilder().setName("ListOfTwoInt32").setSampleType(SampleType::Int32).setDimensions(dimensions).build()))
            .setDimensions(dimensions)
            .build();

    // Set the descriptor, thereby adding the struct type to the type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<int32_t*>(dataPacket.getData());
    data[16] = 1;
    data[17] = 2;
    data[18] = 3;
    data[19] = 4;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lv = signal.getLastValue();

    // Create data structure
    ListPtr<IStruct> ptr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(ptr = lv.asPtr<IList>());

    // Check all inner members of the list
    const auto first = static_cast<ListPtr<IInteger>>(ptr.getItemAt(0).get("ListOfTwoInt32")).getItemAt(0);
    const auto second = static_cast<ListPtr<IInteger>>(ptr.getItemAt(0).get("ListOfTwoInt32")).getItemAt(1);
    const auto third = static_cast<ListPtr<IInteger>>(ptr.getItemAt(1).get("ListOfTwoInt32")).getItemAt(0);
    const auto fourth = static_cast<ListPtr<IInteger>>(ptr.getItemAt(1).get("ListOfTwoInt32")).getItemAt(1);
    ASSERT_EQ(first, 1);
    ASSERT_EQ(second, 2);
    ASSERT_EQ(third, 3);
    ASSERT_EQ(fourth, 4);
}

TEST_F(SignalTest, GetLastValueStructOfListsOfStructsOfInt32)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor =
        DataDescriptorBuilder()
            .setName("MyStruct")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(
                DataDescriptorBuilder()
                    .setName("MyList1")
                    .setSampleType(SampleType::Struct)
                    .setStructFields(
                        List<DataDescriptorPtr>(DataDescriptorBuilder().setName("MyInt32").setSampleType(SampleType::Int32).build()))
                    .setDimensions(dimensions)
                    .build(),
                DataDescriptorBuilder()
                    .setName("MyList2")
                    .setSampleType(SampleType::Struct)
                    .setStructFields(
                        List<DataDescriptorPtr>(DataDescriptorBuilder().setName("MyInt32").setSampleType(SampleType::Int32).build()))
                    .setDimensions(dimensions)
                    .build()))
            .build();

    // Set the descriptor, thereby adding the struct type to the type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<int32_t*>(dataPacket.getData());
    data[16] = 1;
    data[17] = 2;
    data[18] = 3;
    data[19] = 4;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lv = signal.getLastValue();

    // Create data structure
    StructPtr ptr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(ptr = lv.asPtr<IStruct>());

    // Check all inner members of the list
    const auto first = static_cast<ListPtr<IStruct>>(ptr.get("MyList1")).getItemAt(0).get("MyInt32");
    const auto second = static_cast<ListPtr<IStruct>>(ptr.get("MyList1")).getItemAt(1).get("MyInt32");
    const auto third = static_cast<ListPtr<IStruct>>(ptr.get("MyList2")).getItemAt(0).get("MyInt32");
    const auto fourth = static_cast<ListPtr<IStruct>>(ptr.get("MyList2")).getItemAt(1).get("MyInt32");
    ASSERT_EQ(first, 1);
    ASSERT_EQ(second, 2);
    ASSERT_EQ(third, 3);
    ASSERT_EQ(fourth, 4);
}

TEST_F(SignalTest, GetLastValueStructWithLists)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(
                DataDescriptorBuilder().setName("Int32").setDimensions(dimensions).setSampleType(SampleType::Int32).build(),
                DataDescriptorBuilder().setName("Float64").setDimensions(dimensions).setSampleType(SampleType::Float64).build()))
            .build();

    // Set the descriptor, thereby adding the struct type to the type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    auto sizeInBytes = 2 * (sizeof(int32_t) + sizeof(double));
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is int32_t
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<int32_t*>(b);
    *B = 13;

    // Third member of data is double
    void* c = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(int32_t);
    auto C = static_cast<double*>(c);
    *C = 15.1;

    // Fourth member of data is double
    void* d = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(int32_t) + sizeof(double);
    auto D = static_cast<double*>(d);
    *D = 16.1;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lv = signal.getLastValue();

    // Create data structure
    StructPtr ptr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(ptr = lv.asPtr<IStruct>());

    // Get lists from struct
    const auto listInt = static_cast<ListPtr<IInteger>>(ptr.get("Int32"));
    const auto listDouble = static_cast<ListPtr<IFloat>>(ptr.get("Float64"));

    // Check all inner members of the list
    const auto first = listInt.getItemAt(0);
    const auto second = listInt.getItemAt(1);
    const auto third = listDouble.getItemAt(0);
    const auto fourth = listDouble.getItemAt(1);
    ASSERT_EQ(first, 12);
    ASSERT_EQ(second, 13);
    ASSERT_DOUBLE_EQ(third, 15.1);
    ASSERT_DOUBLE_EQ(fourth, 16.1);
}
TEST_F(SignalTest, GetLastValueStructNoSetDescriptor)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    const auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build(),
                                                     DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build()))
            .build();

    // Prepare data packet
    auto sizeInBytes = sizeof(int32_t) + sizeof(double);
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is double
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    // Throws becuase we didn't use signal.setDescriptor
    ASSERT_THROW(signal.getLastValue(), NotFoundException);
}

TEST_F(SignalTest, GetLastValueStructNested)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    const auto descriptor = DataDescriptorBuilder()
                                .setName("MyTestStructType")
                                .setSampleType(SampleType::Struct)
                                .setStructFields(List<DataDescriptorPtr>(
                                    DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build(),
                                    DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build(),
                                    DataDescriptorBuilder()
                                        .setName("Special")
                                        .setSampleType(SampleType::Struct)
                                        .setStructFields(List<DataDescriptorPtr>(
                                            DataDescriptorBuilder().setName("NestedInt64").setSampleType(SampleType::Int64).build()))
                                        .build()))
                                .build();

    // Set the descriptor, thereby adding the struct type in type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    auto sizeInBytes = sizeof(int32_t) + sizeof(double) + sizeof(int64_t);
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is double
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;

    // Third member is nested within Struct and is int64_t
    void* c = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double);
    auto C = static_cast<int64_t*>(c);
    *C = 42;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lastValuePacket = signal.getLastValue();

    // Create data structure
    StructPtr structPtr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(structPtr = lastValuePacket.asPtr<IStruct>());

    // Check first member
    ASSERT_EQ(structPtr.get("Int32"), 12);

    // Check second member
    ASSERT_DOUBLE_EQ(structPtr.get("Float64"), 15.1);

    // Get nested Struct
    StructPtr nestedPtr = structPtr.get("Special").asPtr<IStruct>();

    // Check third (nested) member
    ASSERT_EQ(nestedPtr.get("NestedInt64"), 42);
}

TEST_F(SignalTest, GetLastValueStructDoublyNested)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create data descriptor
    const auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(
                DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build(),
                DataDescriptorBuilder().setName("Float64").setSampleType(SampleType::Float64).build(),
                DataDescriptorBuilder()
                    .setName("Special1")
                    .setSampleType(SampleType::Struct)
                    .setStructFields(List<DataDescriptorPtr>(
                        DataDescriptorBuilder().setName("NestedInt64").setSampleType(SampleType::Int64).build(),
                        DataDescriptorBuilder().setName("NestedInt32").setSampleType(SampleType::Int32).build(),
                        DataDescriptorBuilder()
                            .setName("Special2")
                            .setSampleType(SampleType::Struct)
                            .setStructFields(List<DataDescriptorPtr>(
                                DataDescriptorBuilder().setName("DoublyNestedFloat32").setSampleType(SampleType::Float32).build()))
                            .build()))
                    .build()))
            .build();

    // Set the descriptor, thereby adding the struct type in type manager
    signal.setDescriptor(descriptor);

    // Prepare data packet
    auto sizeInBytes = sizeof(int32_t) + sizeof(double) + sizeof(int64_t) + sizeof(int32_t) + sizeof(float);
    const auto dataPacket = DataPacket(descriptor, 5);
    auto data = dataPacket.getData();

    // Start points to beggining of data
    auto start = static_cast<char*>(data);

    // First member of data is int32_t
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;

    // Second member of data is double
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;

    // Third member is nested within Struct and is int64_t
    void* c = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double);
    auto C = static_cast<int64_t*>(c);
    *C = 42;

    // Fouth member of data is int32_t
    void* d = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double) + sizeof(int64_t);
    auto D = static_cast<int32_t*>(d);
    *D = 33;

    // Fifth member of data is doubly nested withing Stuct and is double
    void* e = start + sizeInBytes * 4 + sizeof(int32_t) + sizeof(double) + sizeof(int64_t) + sizeof(int32_t);
    auto E = static_cast<float*>(e);
    *E = 6.66f;

    // Send our packet
    signal.sendPacket(dataPacket);

    // Call getLastValue
    auto lastValuePacket = signal.getLastValue();

    // Create data structure
    StructPtr structPtr;

    // Cast lastValuePacket to our data structure and ASSERT_NO_THROW
    ASSERT_NO_THROW(structPtr = lastValuePacket.asPtr<IStruct>());

    // Check first member
    ASSERT_EQ(structPtr.get("Int32"), 12);

    // Check second member
    ASSERT_DOUBLE_EQ(structPtr.get("Float64"), 15.1);

    // Get nested Struct
    StructPtr nestedPtr = structPtr.get("Special1").asPtr<IStruct>();

    // Check third (nested) member
    ASSERT_EQ(nestedPtr.get("NestedInt64"), 42);

    // Check fourth (nested) member
    ASSERT_EQ(nestedPtr.get("NestedInt32"), 33);

    // Get doubly nested Struct
    StructPtr doublyNestedPtr = nestedPtr.get("Special2").asPtr<IStruct>();

    // Check fifth (doubly nested) member
    ASSERT_FLOAT_EQ(doublyNestedPtr.get("DoublyNestedFloat32"), 6.66f);
}
