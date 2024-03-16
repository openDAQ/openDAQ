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
#include <opendaq/input_port_notifications.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/scheduler_factory.h>
#include <thread>

using SignalTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

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

    ErrCode INTERFACE_FUNC enqueueAndSteal(IPacket* packet) override
    {
        packetsEnqueued++;
        packetEnqueued = true;

        if (packet != nullptr)
            packet->releaseRef();

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enqueueAndStealMultiple(IList* packets) override
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
    checkErrorInfo(signal->sendAndStealPacket(packet.detach()));

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

    checkErrorInfo(signal->sendAndStealPackets(packets.detach()));

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
    signal.template asPtr<IComponentPrivate>().unlockAttributes(List<IString>("visible"));
    signal.setVisible(False);
    signal.template asPtr<IComponentPrivate>().lockAttributes(List<IString>("visible"));

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
    const auto ctx = Context(Scheduler(logger), logger, TypeManager(), nullptr);

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
                    ASSERT_EQ(packet.getRefCount(), 1);
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
    checkErrorInfo(signal->sendAndStealPacket(packet.detach()));

    ASSERT_EQ(dataPacketsRecieved, 1);
}

TEST_F(SignalTest, LastReferenceScheduler)
{
    std::mutex mtx;
    std::condition_variable cv;
    bool packetRead = false;
    std::thread::id threadId;

    const auto logger = Logger();
    const auto ctx = Context(Scheduler(logger), logger, TypeManager(), nullptr);

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
                    ASSERT_EQ(packet.getRefCount(), 1);

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
    checkErrorInfo(signal->sendAndStealPacket(packet.detach()));

    {
        std::unique_lock lock(mtx);
        while (!packetRead)
            cv.wait(lock);
    }

    ASSERT_EQ(dataPacketsRecieved, 1);
    ASSERT_NE(std::this_thread::get_id(), threadId);
}

TEST_F(SignalTest, LastReferenceSameThreadMultiPackets)
{
    const auto logger = Logger();
    const auto ctx = Context(Scheduler(logger), logger, TypeManager(), nullptr);

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
                    ASSERT_EQ(packet.getRefCount(), 1);
                }
                packet = conn.dequeue();
            }
        });

    const auto ip = InputPort(ctx, nullptr, "ip");
    ip.setNotificationMethod(PacketReadyNotification::SameThread);
    ip.setListener(listener);

    ip.connect(signal);

    auto packets = List<IPacket>(DataPacket(descriptor, 1), DataPacket(descriptor, 1));

    checkErrorInfo(signal->sendAndStealPackets(packets.detach()));

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

END_NAMESPACE_OPENDAQ
