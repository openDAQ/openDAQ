#include <coreobjects/property_object_class_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/removable_ptr.h>
#include <opendaq/signal_events.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/tags_factory.h>
#include <opendaq/input_port_factory.h>

using SignalTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

class ConnectionMockImpl : public ImplementationOf<IConnection>
{
public:
    bool packetEnqueued{false};

    ErrCode INTERFACE_FUNC enqueue(IPacket* packet) override
    {
        packetEnqueued = true;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enqueueOnThisThread(IPacket* packet) override
    {
        packetEnqueued = true;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC dequeue(IPacket** packet) override
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
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2);

    signal.setActive(false);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2);

    auto descriptor1 = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int16).build();
    signal.setDescriptor(descriptor1);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 3);
}

TEST_F(SignalTest, TestInputPortActiveSendPacket)
{
    const auto context = NullContext();
    const auto signal = Signal(context, nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Float64).build();
    const auto ip = InputPort(context, nullptr, "ip");
    ip.connect(signal);

    auto dataPacket = DataPacket(descriptor, 1);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2);

    ip.setActive(false);
    signal.sendPacket(dataPacket);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 2);

    auto descriptor1 = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int16).build();
    signal.setDescriptor(descriptor1);
    ASSERT_EQ(ip.getConnection().getPacketCount(), 3);
}

TEST_F(SignalTest, GetLastValueStruct)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create DataDescriptor
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

TEST_F(SignalTest, GetLastValueStructNested)
{
    // Create signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // Create DataDescriptor
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

    // Create DataDescriptor
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

END_NAMESPACE_OPENDAQ
