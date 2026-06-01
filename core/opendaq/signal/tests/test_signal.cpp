#include <coreobjects/property_object_class_factory.h>
#include <gtest/gtest.h>
#include <testutils/testutils.h>
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
#include <coreobjects/property_factory.h>
#include <opendaq/binary_data_packet_factory.h>

using SignalTest = testing::Test;

using namespace daq;

class ConnectionMockImpl : public ImplementationOf<IConnection>
{
public:
    bool packetEnqueued{ false };
    SizeT packetsEnqueued{0};

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

    ErrCode INTERFACE_FUNC enqueueWithScheduler(IPacket* packet) override
    {
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
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
    }

    ErrCode INTERFACE_FUNC getRefCount(SizeT* refCount) override
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
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

    ErrCode INTERFACE_FUNC getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo) override
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

    signal.setDescriptor(nullptr);
    ASSERT_FALSE(signal.getDescriptor().assigned());
}

TEST_F(SignalTest, SignalNullTypeDescriptor)
{
    SignalConfigPtr signal;
    const auto nullDescriptor = NullDataDescriptor();

    ASSERT_THROW_MSG(signal = SignalWithDescriptor(NullContext(), nullDescriptor, nullptr, "sig"),
                     InvalidSampleTypeException,
                     "SampleType \"Null\" is reserved for \"DATA_DESCRIPTOR_CHANGED\" event packet.");

    ASSERT_NO_THROW(signal = SignalWithDescriptor(NullContext(), nullptr, nullptr, "sig"));
    ASSERT_THROW_MSG(signal.setDescriptor(nullDescriptor),
                     InvalidSampleTypeException,
                     "SampleType \"Null\" is reserved for \"DATA_DESCRIPTOR_CHANGED\" event packet.");

    ASSERT_FALSE(signal.getDescriptor().assigned());
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

    // Throws because descriptor2 has the same name as descriptor1 (but is different)
    // and hence the the struct type can't be added to the type manager and hence
    // later can't be found in the type manager

    // Note: the calculation of last value is moved to the method getLastValue
    // ASSERT_THROW(signal2.sendPacket(dataPacket2), NotFoundException);
    ASSERT_NO_THROW(signal2.sendPacket(dataPacket2));
    ASSERT_THROW(signal2.getLastValue(), NotFoundException);
    daq::BaseObjectPtr value;
    daq::BaseObjectPtr timestamp;
    ASSERT_THROW(timestamp = signal2.getLastValueWithTimestamp(value), NotFoundException);

    {
        const auto lv1 = signal1.getLastValue();
        StructPtr sp1;
        ASSERT_NO_THROW(sp1 = lv1.asPtr<IStruct>());
        ASSERT_EQ(sp1.get("Int32"), 4);
    }
    {
        daq::BaseObjectPtr lv1;
        daq::BaseObjectPtr ts = signal1.getLastValueWithTimestamp(lv1);
        StructPtr sp1;
        ASSERT_NO_THROW(sp1 = lv1.asPtr<IStruct>());
        ASSERT_EQ(sp1.get("Int32"), 4);
        ASSERT_FALSE(ts.assigned());
    }
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
    ASSERT_EQ(connImpl->packetsEnqueued, 3u);

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

    ASSERT_EQ(connImpl->packetsEnqueued, 3u);
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

    // Lock keeps Public unchanged, but doesn't throw exceptions
    ASSERT_NO_THROW(signal.setPublic(true));
    ASSERT_EQ(signal.getPublic(), false);
}

TEST_F(SignalTest, NoLastValue)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    ASSERT_FALSE(signal.getLastValue().assigned());
}

TEST_F(SignalTest, SetLastValue)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    signal.setLastValue(4);
    ASSERT_EQ(signal.getLastValue(), 4);

    BaseObjectPtr value;
    BaseObjectPtr ts;
    ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
    ASSERT_TRUE(value.assigned());
    ASSERT_FALSE(ts.assigned());
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

TEST_F(SignalTest, GetLastValueString)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::String).build();

    // Allocate buffer for "abcd" (4 bytes, no null terminator needed)
    auto dataPacket = BinaryDataPacket(nullptr, descriptor, 4);
    char* data = static_cast<char*>(dataPacket.getData());
    data[0] = 'a';
    data[1] = 'b';
    data[2] = 'c';
    data[3] = 'd';

    signal.sendPacket(dataPacket);

    auto lastValue = signal.getLastValue();
    StringPtr ptr;
    ASSERT_NO_THROW(ptr = lastValue.asPtr<IString>());
    ASSERT_EQ(ptr, "abcd");
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

TEST_F(SignalTest, EnableKeepLastValue)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    const auto privateSignal = signal.asPtrOrNull<ISignalPrivate>();
    ASSERT_TRUE(privateSignal.getKeepLastValue());
    privateSignal.enableKeepLastValue(false);
    ASSERT_FALSE(privateSignal.getKeepLastValue());
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

TEST_F(SignalTest, GetLastValueInvisible)
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

    ASSERT_TRUE(signal.getLastValue().assigned());
}

class SignalLastValueWithTimestampTest : public testing::Test
{
public:

    void SetUp() override
    {
        Init();
        Check();
    }

    void TearDown() override
    {
        signal.release();
        domainSignal.release();
    }

    void Init()
    {
        signal = Signal(NullContext(), nullptr, "sig");
        domainSignal = Signal(NullContext(), nullptr, "domainSig");
    }

    template <typename T>
    DataPacketPtr BuildDataPacket(const DataDescriptorPtr& desc, const DataPacketPtr& domainPacket, const size_t packetSize, const T value)
    {
        auto dataPacket = DataPacketWithDomain(domainPacket, desc, packetSize);
        static_cast<T*>(dataPacket.getData())[packetSize - 1] = value;
        return dataPacket;
    }

    template <typename T>
    DataPacketPtr BuildDomainPacket(const DataDescriptorPtr& desc, const size_t packetSize, const T ts)
    {
        auto domainPacket = DataPacket(desc, packetSize);
        static_cast<T*>(domainPacket.getData())[packetSize - 1] = ts;
        return domainPacket;
    }

    void Check()
    {
        Check(BaseObjectPtr{});
    }

    void Check(const BaseObjectPtr& expectedValue)
    {
        Check(expectedValue, -1);
    }

    void Check(const BaseObjectPtr& expectedValue, const int64_t expectedTs)
    {
        BaseObjectPtr value;
        BaseObjectPtr ts;
        ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
        if (expectedValue.assigned())
        {
            ASSERT_TRUE(value.assigned());
            ASSERT_EQ(value, expectedValue);
        }
        else
        {
            ASSERT_FALSE(value.assigned());
        }

        if (expectedTs >= 0)
        {
            ASSERT_TRUE(ts.assigned());
            IntegerPtr tsPtr;
            ASSERT_NO_THROW(tsPtr = ts.asPtr<IInteger>());
            ASSERT_EQ(tsPtr, expectedTs);
        }
        else
        {
            ASSERT_FALSE(ts.assigned());
        }
    }

    SignalConfigPtr signal;
    SignalConfigPtr domainSignal;
};

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampAfterMultipleSend)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    {
        auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(1779961693));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(10));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);
    }

    {
        auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(1779964000));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(20));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);
    }
    Check(Integer(20), 1779964000000000);
}

TEST_F(SignalLastValueWithTimestampTest, NoLastValueWithTimestamp)
{
    BaseObjectPtr value;
    BaseObjectPtr ts;
    ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
    ASSERT_FALSE(value.assigned());
    ASSERT_FALSE(ts.assigned());
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampNotAssigned)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto dataPacket = DataPacket(descriptor, 5);

    auto* data = static_cast<int64_t*>(dataPacket.getData());
    data[4] = 41;

    signal.sendPacket(dataPacket);

    Check(Integer(41));
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampAssigned)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int64_t(1779961600));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(45));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(45), 1779961600000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampWrongTsSampleType)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::ComplexFloat64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = DataPacket(domainDescriptor, 5);
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(85));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(85));
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampWrongTsUnit)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("pps"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int64_t(1779961695));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(65));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(65));
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampWitoutTsUnit)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int64_t(1779961696));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(789));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(789));
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampWrongOrigin)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("wrongOrigin")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int64_t(1779961691));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(654));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(654));
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampTsDescChangedToWrong)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    {
        auto domainDescriptor = DataDescriptorBuilder()
                                    .setName("tsSig")
                                    .setSampleType(SampleType::Int64)
                                    .setUnit(Unit("s"))
                                    .setOrigin("1970-01-01T00:00:00")
                                    .build();
        auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int64_t(1779961999));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(-5));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(-5), 1779961999000000);
    }

    {
        auto wrongDomainDescriptor =
            DataDescriptorBuilder().setName("tsSig").setSampleType(SampleType::Int64).setOrigin("1970-01-01T00:00:00").build();
        auto domainPacket = BuildDomainPacket(wrongDomainDescriptor, 2, int64_t(1779962000));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 2, int64_t(56));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(56));
    }
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampTsOriginChanged)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    {
        auto domainDescriptor = DataDescriptorBuilder()
        .setName("tsSig")
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s"))
            .setOrigin("1970-01-01T00:00:00")
            .build();
        auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(1779961999));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(-5));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(-5), 1779961999000000);
    }

    {
        auto wrongDomainDescriptor = DataDescriptorBuilder()
        .setName("tsSig")
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s"))
            .setOrigin("wrong")
            .build();
        auto domainPacket = BuildDomainPacket(wrongDomainDescriptor, 1, int64_t(1779962000));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(56));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(56));
    }

    {
        auto domainDescriptor = DataDescriptorBuilder()
        .setName("tsSig")
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s"))
            .setOrigin("1970-01-01T00:00:00")
            .build();
        auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(1779962001));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(-52));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(-52), 1779962001000000);
    }
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampString)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::String).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setTickResolution(Ratio(1, 1000))
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(1779961693000));

    // Allocate buffer for "abcd" (4 bytes, no null terminator needed)
    auto dataPacket = BinaryDataPacket(domainPacket, descriptor, 4);
    char* data = static_cast<char*>(dataPacket.getData());
    data[0] = 'a';
    data[1] = 'b';
    data[2] = 'c';
    data[3] = 'd';


    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(String("abcd"), 1779961693000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampUInt64)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::UInt64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, uint64_t(1778861693ULL));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(-45));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(-45), 1778861693000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampDouble)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Float64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, double(1777761693.0));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(896));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(896), 1777761693000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampFloat)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Float32)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, float(177991.0));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(735));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(735), 177991000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampInt32)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int32)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int32_t(1279961693));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(4));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(4), 1279961693000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampUInt32)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::UInt32)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, uint32_t(1239961693));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(99));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(99), 1239961693000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampInt16)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int16)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int16_t(21365));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(29));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(29), 21365000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampUInt16)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::UInt16)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, uint16_t(51365));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(31));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(31), 51365000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampInt8)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int8)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int8_t(123));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(29));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(29), 123000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampUInt8)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::UInt8)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, uint8_t(221));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(31));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(31), 221000000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampDomainHasDimensions)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setDimensions(dimensions)
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = DataPacket(domainDescriptor, 5);
    int64_t* domainData = static_cast<int64_t*>(domainPacket.getData());
    domainData[8] = 1779961693;
    domainData[9] = 1779961694;

    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(-852));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(-852));
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampTsDescChangedToValid)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();

    {
        auto invalidDomainDescriptor =
            DataDescriptorBuilder().setName("tsSig").setSampleType(SampleType::Int64).setOrigin("1970-01-01T00:00:00").build();

        auto domainPacket = BuildDomainPacket(invalidDomainDescriptor, 1, int64_t(1771661693));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(4));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(4));
    }

    {
        auto validDomainDescriptor = DataDescriptorBuilder()
                                         .setName("tsSig")
                                         .setSampleType(SampleType::Int64)
                                         .setUnit(Unit("s"))
                                         .setOrigin("1970-01-01T00:00:00")
                                         .build();

        auto domainPacket = BuildDomainPacket(validDomainDescriptor, 1, int64_t(1771661695));
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(-56));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(-56), 1771661695000000);
    }
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampDisabled)
{
    const auto privateSignal = signal.asPtrOrNull<ISignalPrivate>();
    ASSERT_TRUE(privateSignal.assigned());
    privateSignal.enableKeepLastValue(false);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(10));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(56));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check();
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampNonPublicDisabled)
{
    signal.setPublic(False);

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 1, int64_t(11));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 1, int64_t(57));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check();
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampLinearRuleBasic)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setRule(LinearDataRule(2, 10))
                                .build();

    // Last tick = 2*4 + 10 + 0 = 18
    auto domainPacket = DataPacket(domainDescriptor, 5, 0);
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(123));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(123), 18'000'000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampLinearRuleWithOffset)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setRule(LinearDataRule(3, 5))
                                .build();

    // Last tick = 3*3 + 5 + 100 = 114
    auto domainPacket = DataPacket(domainDescriptor, 4, 100);
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 4, int64_t(-77));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(-77), 114'000'000);
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampLinearRuleMultipleSend)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setRule(LinearDataRule(1, 0))
                                .build();

    {
        // Last tick = 1*9 + 0 + 0 = 9
        auto domainPacket = DataPacket(domainDescriptor, 10, 0);
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 10, int64_t(10));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(10), 9'000'000);
    }

    {
        // Last tick = 1*9 + 0 + 10 = 19
        auto domainPacket = DataPacket(domainDescriptor, 10, 10);
        auto dataPacket = BuildDataPacket(descriptor, domainPacket, 10, int64_t(20));

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        Check(Integer(20), 19'000'000);
    }
}

TEST_F(SignalLastValueWithTimestampTest, GetLastValueWithTimestampLinearRuleTickResolution)
{
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setTickResolution(Ratio(1, 1000))
                                .setRule(LinearDataRule(1000, 1'779'961'695'000))
                                .build();

    // Last tick = 1000*4 + 1'779'961'695'000 + 1'000 = 1'779'961'700'000; with resolution 1/1000 -> us = tick*1000
    auto domainPacket = DataPacket(domainDescriptor, 5, 1'000);
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(456));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(456), 1'779'961'700'000'000);
}

class SignalLastValueWithTimestampPTest : public ::testing::TestWithParam<std::pair<std::string, int64_t>>
{
public:

    static std::string ParamNameGenerator(const testing::TestParamInfo<std::pair<std::string, int64_t>>& info)
    {
        std::string str(info.param.first);
        std::replace(str.begin(), str.end(), '-', '_');
        std::replace(str.begin(), str.end(), ' ', '_');
        std::replace(str.begin(), str.end(), '/', '_');
        std::replace(str.begin(), str.end(), ':', '_');
        std::replace(str.begin(), str.end(), '+', 'p');
        std::replace(str.begin(), str.end(), '-', 'm');
        return str;
    }

    void SetUp() override
    {
        Init();
        Check();
    }

    void TearDown() override
    {
        signal.release();
        domainSignal.release();
    }

    void Init()
    {
        signal = Signal(NullContext(), nullptr, "sig");
        domainSignal = Signal(NullContext(), nullptr, "domainSig");
    }

    template <typename T>
    DataPacketPtr BuildDataPacket(const DataDescriptorPtr& desc, const DataPacketPtr& domainPacket, const size_t packetSize, const T value)
    {
        auto dataPacket = DataPacketWithDomain(domainPacket, desc, packetSize);
        static_cast<T*>(dataPacket.getData())[packetSize - 1] = value;
        return dataPacket;
    }

    template <typename T>
    DataPacketPtr BuildDomainPacket(const DataDescriptorPtr& desc, const size_t packetSize, const T ts)
    {
        auto domainPacket = DataPacket(desc, packetSize);
        static_cast<T*>(domainPacket.getData())[packetSize - 1] = ts;
        return domainPacket;
    }

    void Check()
    {
        Check(BaseObjectPtr{});
    }

    void Check(const BaseObjectPtr& expectedValue)
    {
        Check(expectedValue, -1);
    }

    void Check(const BaseObjectPtr& expectedValue, const int64_t expectedTs)
    {
        BaseObjectPtr value;
        BaseObjectPtr ts;
        ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
        if (expectedValue.assigned())
        {
            ASSERT_TRUE(value.assigned());
            ASSERT_EQ(value, expectedValue);
        }
        else
        {
            ASSERT_FALSE(value.assigned());
        }

        if (expectedTs >= 0)
        {
            ASSERT_TRUE(ts.assigned());
            IntegerPtr tsPtr;
            ASSERT_NO_THROW(tsPtr = ts.asPtr<IInteger>());
            ASSERT_EQ(tsPtr, expectedTs);
        }
        else
        {
            ASSERT_FALSE(ts.assigned());
        }
    }

    SignalConfigPtr signal;
    SignalConfigPtr domainSignal;
};

TEST_P(SignalLastValueWithTimestampPTest, GetLastValueWithTimestampWithDifferentOrigins)
{
    constexpr int64_t baseTs = 1779961693;
    const auto [origin, offset] = GetParam();
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin(origin.c_str())
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, int64_t(baseTs));
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(147896325));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(147896325), baseTs * 1000000 + offset);
}

INSTANTIATE_TEST_SUITE_P(GetLastValueWithTimestampWithDifferentOrigins,
                         SignalLastValueWithTimestampPTest,
                         ::testing::Values(std::pair<std::string, int64_t>("1970-01-01T00:00:00", 0),
                                           std::pair<std::string, int64_t>("1970-01-01T00:00:00Z", 0),
                                           std::pair<std::string, int64_t>("1970-01-01", 0),
                                           std::pair<std::string, int64_t>("1970-01-01T00:00:00+01:00", -3'600'000'000),
                                           std::pair<std::string, int64_t>("1970-01-01T00:00:00-02:00", 7'200'000'000),
                                           std::pair<std::string, int64_t>("1970-01-01T00:00:02", 2'000'000),
                                           std::pair<std::string, int64_t>("1970-01-01T00:02:01", 121'000'000),
                                           std::pair<std::string, int64_t>("1970-01-02T00:00:00", 86'400'000'000),
                                           std::pair<std::string, int64_t>("1970-02-01T00:00:00", 2'678'400'000'000),
                                           std::pair<std::string, int64_t>("1971-01-01T00:00:00", 31'536'000'000'000),
                                           std::pair<std::string, int64_t>("1971-01-01T00:00:00Z", 31'536'000'000'000),
                                           std::pair<std::string, int64_t>("2000-01-01T00:00:00", 946'684'800'000'000)),
                         SignalLastValueWithTimestampPTest::ParamNameGenerator);

struct TickResolutionParam
{
    int64_t numerator;
    int64_t denominator;
    int64_t tick;
    int64_t expectedMicros;
};

class SignalLastValueWithTickResolutionPTest : public ::testing::TestWithParam<TickResolutionParam>
{
public:

    static std::string ParamNameGenerator(const testing::TestParamInfo<TickResolutionParam>& info)
    {
        return "Resolution_" + std::to_string(info.param.numerator) + "_" + std::to_string(info.param.denominator);
    }

    void SetUp() override
    {
        Init();
        Check();
    }

    void TearDown() override
    {
        signal.release();
        domainSignal.release();
    }

    void Init()
    {
        signal = Signal(NullContext(), nullptr, "sig");
        domainSignal = Signal(NullContext(), nullptr, "domainSig");
    }

    template <typename T>
    DataPacketPtr BuildDataPacket(const DataDescriptorPtr& desc, const DataPacketPtr& domainPacket, const size_t packetSize, const T value)
    {
        auto dataPacket = DataPacketWithDomain(domainPacket, desc, packetSize);
        static_cast<T*>(dataPacket.getData())[packetSize - 1] = value;
        return dataPacket;
    }

    template <typename T>
    DataPacketPtr BuildDomainPacket(const DataDescriptorPtr& desc, const size_t packetSize, const T ts)
    {
        auto domainPacket = DataPacket(desc, packetSize);
        static_cast<T*>(domainPacket.getData())[packetSize - 1] = ts;
        return domainPacket;
    }

    void Check()
    {
        Check(BaseObjectPtr{});
    }

    void Check(const BaseObjectPtr& expectedValue)
    {
        Check(expectedValue, -1);
    }

    void Check(const BaseObjectPtr& expectedValue, const int64_t expectedTs)
    {
        BaseObjectPtr value;
        BaseObjectPtr ts;
        ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
        if (expectedValue.assigned())
        {
            ASSERT_TRUE(value.assigned());
            ASSERT_EQ(value, expectedValue);
        }
        else
        {
            ASSERT_FALSE(value.assigned());
        }

        if (expectedTs >= 0)
        {
            ASSERT_TRUE(ts.assigned());
            IntegerPtr tsPtr;
            ASSERT_NO_THROW(tsPtr = ts.asPtr<IInteger>());
            ASSERT_EQ(tsPtr, expectedTs);
        }
        else
        {
            ASSERT_FALSE(ts.assigned());
        }
    }

    SignalConfigPtr signal;
    SignalConfigPtr domainSignal;
};

TEST_P(SignalLastValueWithTickResolutionPTest, GetLastValueWithTimestampWithDifferentTickResolutions)
{
    const auto [num, den, tick, expectedTs] = GetParam();
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setTickResolution(Ratio(num, den))
                                .build();

    auto domainPacket = BuildDomainPacket(domainDescriptor, 5, tick);
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, 5, int64_t(147896325));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(147896325), expectedTs);
}

INSTANTIATE_TEST_SUITE_P(GetLastValueWithTimestampWithDifferentTickResolutions,
                         SignalLastValueWithTickResolutionPTest,
                         ::testing::Values(TickResolutionParam{1, 1, 1'779'961'693, 1'779'961'693'000'000},
                                           TickResolutionParam{1, 100, 177'996'169'400, 1'779'961'694'000'000},
                                           TickResolutionParam{1, 1'000, 1'779'961'695'000, 1'779'961'695'000'000},
                                           TickResolutionParam{1, 10'000, 17'799'616'960'000, 1'779'961'696'000'000},
                                           TickResolutionParam{1, 1'000'000, 1'779'961'697'000'000, 1'779'961'697'000'000},
                                           TickResolutionParam{1, 10'000'000, 17'799'616'980'000'000, 1'779'961'698'000'000},
                                           TickResolutionParam{1, 1'000'000'000, 17'799'000'000'000, 17'799'000'000},
                                           TickResolutionParam{3, 1'000, 1'000'000, 3'000'000'000},
                                           TickResolutionParam{7, 1'000'000, 1'000'000, 7'000'000},
                                           TickResolutionParam{3, 7, 7'000, 3'000'000'000},
                                           TickResolutionParam{11, 1'000, 1'000'000, 11'000'000'000}),
                         SignalLastValueWithTickResolutionPTest::ParamNameGenerator);

struct LinearRuleParam
{
    int64_t delta;
    int64_t start;
    int64_t packetOffset;
    int64_t sampleCount;
    int64_t expectedMicros;  // (delta * (sampleCount - 1) + start + packetOffset) * 1'000'000
};

class SignalLastValueLinearRulePTest : public ::testing::TestWithParam<LinearRuleParam>
{
public:

    static std::string ParamNameGenerator(const testing::TestParamInfo<LinearRuleParam>& info)
    {
        const auto& p = info.param;
        return "Linear_d" + std::to_string(p.delta) + "_s" + std::to_string(p.start) + "_o" +
               std::to_string(p.packetOffset) + "_n" + std::to_string(p.sampleCount);
    }

    void SetUp() override
    {
        Init();
        Check();
    }

    void TearDown() override
    {
        signal.release();
        domainSignal.release();
    }

    void Init()
    {
        signal = Signal(NullContext(), nullptr, "sig");
        domainSignal = Signal(NullContext(), nullptr, "domainSig");
    }

    template <typename T>
    DataPacketPtr BuildDataPacket(const DataDescriptorPtr& desc, const DataPacketPtr& domainPacket, const size_t packetSize, const T value)
    {
        auto dataPacket = DataPacketWithDomain(domainPacket, desc, packetSize);
        static_cast<T*>(dataPacket.getData())[packetSize - 1] = value;
        return dataPacket;
    }

    void Check()
    {
        Check(BaseObjectPtr{});
    }

    void Check(const BaseObjectPtr& expectedValue)
    {
        Check(expectedValue, -1);
    }

    void Check(const BaseObjectPtr& expectedValue, const int64_t expectedTs)
    {
        BaseObjectPtr value;
        BaseObjectPtr ts;
        ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
        if (expectedValue.assigned())
        {
            ASSERT_TRUE(value.assigned());
            ASSERT_EQ(value, expectedValue);
        }
        else
        {
            ASSERT_FALSE(value.assigned());
        }

        if (expectedTs >= 0)
        {
            ASSERT_TRUE(ts.assigned());
            IntegerPtr tsPtr;
            ASSERT_NO_THROW(tsPtr = ts.asPtr<IInteger>());
            ASSERT_EQ(tsPtr, expectedTs);
        }
        else
        {
            ASSERT_FALSE(ts.assigned());
        }
    }

    SignalConfigPtr signal;
    SignalConfigPtr domainSignal;
};

TEST_P(SignalLastValueLinearRulePTest, GetLastValueWithTimestampLinearRule)
{
    const auto [delta, start, packetOffset, sampleCount, expectedTs] = GetParam();
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto domainDescriptor = DataDescriptorBuilder()
                                .setName("tsSig")
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s"))
                                .setOrigin("1970-01-01T00:00:00")
                                .setRule(LinearDataRule(delta, start))
                                .build();

    auto domainPacket = DataPacket(domainDescriptor, sampleCount, packetOffset);
    auto dataPacket = BuildDataPacket(descriptor, domainPacket, sampleCount, int64_t(42));

    domainSignal.sendPacket(domainPacket);
    signal.sendPacket(dataPacket);

    Check(Integer(42), expectedTs);
}

INSTANTIATE_TEST_SUITE_P(GetLastValueWithTimestampLinearRule,
                         SignalLastValueLinearRulePTest,
                         ::testing::Values(LinearRuleParam{1, 0, 0, 5, 4'000'000},
                                           LinearRuleParam{2, 10, 0, 5, 18'000'000},
                                           LinearRuleParam{3, 5, 100, 4, 114'000'000},
                                           LinearRuleParam{5, 0, 0, 1, 0},
                                           LinearRuleParam{10, 2, 50, 8, 122'000'000},
                                           LinearRuleParam{1, 1'000'000, 0, 5, 1'000'004'000'000}),
                         SignalLastValueLinearRulePTest::ParamNameGenerator);

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
    ASSERT_DOUBLE_EQ(complexPtr.getReal(), 8.1f);
    ASSERT_DOUBLE_EQ(complexPtr.getImaginary(), 9.1f);
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

TEST_F(SignalTest, GetLastValueConstantDataRule)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setRule(ConstantDataRule()).setName("test").setSampleType(SampleType::Int64).build();

    auto dataPacket = ConstantDataPacketWithDomain<Int>(nullptr,
                                                                         descriptor,
                                                                         100,
                                                                         12,
                                                                         {{10, 16}, {70, 18}, {90, 20}});

    signal.sendPacket(dataPacket);

    auto lastValuePacket = signal.getLastValue();
    IntegerPtr ptr;
    ASSERT_NO_THROW(ptr = lastValuePacket.asPtr<IInteger>());
    ASSERT_EQ(ptr, 20);
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
    // Throws becuase we didn't use signal.setDescriptor

    // TODO: we optimized the getLastValue, so calculation of last value moved to the the method getLastValue of the signal
    // ASSERT_THROW(signal.sendPacket(dataPacket), NotFoundException);
    ASSERT_NO_THROW(signal.sendPacket(dataPacket));
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

TEST_F(SignalTest, SetDomainDescriptorUnderLock)
{
    auto context = NullContext();
    auto desc = DataDescriptorBuilder().build();
    auto desc1 = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();

    const auto signal = Signal(context, nullptr, "sig");
    const auto domainSignal = Signal(context, nullptr, "domainSig");

    signal.setDescriptor(desc);
    domainSignal.setDescriptor(desc);
    signal.setDomainSignal(domainSignal);

    signal.addProperty(IntProperty("Test", 0));
    signal.getOnPropertyValueWrite("Test") += [&domainSignal, &desc, &desc1](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
    {
        if (static_cast<int>(args.getValue()) % 2 == 0)
            ASSERT_NO_THROW(domainSignal.setDescriptor(desc1));
        else
            ASSERT_NO_THROW(domainSignal.setDescriptor(desc));
    };

    for (int i = 0; i < 10; ++i)
        signal.setPropertyValue("Test", i);
}
