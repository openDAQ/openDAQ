#include <testutils/testutils.h>
#include <opendaq/context_factory.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_descriptor_builder_ptr.h>
#include <opendaq/logger_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_factory.h>

#include <future>
#include <chrono>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

class PacketReaderTest : public testing::Test
{
public:
    explicit PacketReaderTest()
        : logger(Logger())
        , context(Context(Scheduler(logger, 1), logger, nullptr, nullptr, nullptr))
        , scheduler(context.getScheduler())
        , signal(Signal(context, nullptr, "sig"))
    {
    }

    void sendPacket(const PacketPtr& packet) const
    {
        signal.sendPacket(packet);
    }

    [[nodiscard]]
    DataDescriptorPtr createDataDescriptor() const
    {
        return DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    }

    void TearDown() override
    {
        scheduler.stop();
        std::this_thread::sleep_for(10ms);
    }

protected:
    LoggerPtr logger;
    ContextPtr context;
    SchedulerPtr scheduler;
    SignalConfigPtr signal;
};

TEST_F(PacketReaderTest, Create)
{
    auto reader = PacketReader(signal);
    scheduler.waitAll();
}

TEST_F(PacketReaderTest, CreateNoSignalThrows)
{
    ASSERT_THROW_MSG(PacketReader(nullptr), ArgumentNullException, "Signal must not be null.")
    scheduler.waitAll();
}

TEST_F(PacketReaderTest, FirstPacketIsEvent)
{
    auto reader = PacketReader(signal);

    // Wait for scheduler to process tasks asynchronously
    scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    auto packet = reader.read();
    ASSERT_EQ(packet.getType(), PacketType::Event);

    scheduler.waitAll();
}

TEST_F(PacketReaderTest, FirstPacketIsDescriptor)
{
    auto reader = PacketReader(signal);

    // Wait for scheduler to process tasks asynchronously
    scheduler.waitAll();

    auto packet = reader.read().asPtrOrNull<IEventPacket>();
    ASSERT_TRUE(packet.assigned());

    ASSERT_EQ(packet.getEventId(), "DATA_DESCRIPTOR_CHANGED");

    auto params = packet.getParameters();
    ASSERT_TRUE(params.hasKey(event_packet_param::DATA_DESCRIPTOR));
    ASSERT_TRUE(params.hasKey(event_packet_param::DOMAIN_DATA_DESCRIPTOR));
}

TEST_F(PacketReaderTest, DataPacket)
{
    signal.setDescriptor(createDataDescriptor());

    auto reader = PacketReader(signal);
    sendPacket(DataPacket(signal.getDescriptor(), 1, 1));

    scheduler.waitAll();

    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 2u);

    auto secondPacket = packets[1];
    ASSERT_EQ(secondPacket.getType(), PacketType::Data);

    auto dataPacket = secondPacket.asPtrOrNull<IDataPacket>(true);
    ASSERT_TRUE(dataPacket.assigned());
}

TEST_F(PacketReaderTest, PacketReaderWithInputPort)
{
    signal.setDescriptor(createDataDescriptor());

    auto port = InputPort(signal.getContext(), nullptr, "readsig");
    auto reader = PacketReaderFromPort(port);
    port.connect(signal);

    sendPacket(DataPacket(signal.getDescriptor(), 1, 1));

    scheduler.waitAll();

    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 2u);

    auto secondPacket = packets[1];
    ASSERT_EQ(secondPacket.getType(), PacketType::Data);

    auto dataPacket = secondPacket.asPtrOrNull<IDataPacket>(true);
    ASSERT_TRUE(dataPacket.assigned());
}

TEST_F(PacketReaderTest, PacketReaderWithNotConnectedInputPort)
{
    signal.setDescriptor(createDataDescriptor());
    auto port = InputPort(signal.getContext(), nullptr, "readsig");

    auto reader = PacketReaderFromPort(port);
    port.connect(signal);
    sendPacket(DataPacket(signal.getDescriptor(), 1, 1));

    scheduler.waitAll();

    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 2u);

    auto secondPacket = packets[1];
    ASSERT_EQ(secondPacket.getType(), PacketType::Data);

    auto dataPacket = secondPacket.asPtrOrNull<IDataPacket>(true);
    ASSERT_TRUE(dataPacket.assigned());
}


TEST_F(PacketReaderTest, MultiplePacketReaderToInputPort)
{
    auto port = InputPort(signal.getContext(), nullptr, "readsig");

    auto reader1 = PacketReaderFromPort(port);
    ASSERT_THROW(PacketReaderFromPort(port), AlreadyExistsException);
}

TEST_F(PacketReaderTest, PacketReaderReuseInputPort)
{
    signal.setDescriptor(createDataDescriptor());
    auto port = InputPort(signal.getContext(), nullptr, "readsig");
    {
        auto reader1 = PacketReaderFromPort(port);
    }
    ASSERT_NO_THROW(PacketReaderFromPort(port));
}

TEST_F(PacketReaderTest, PacketReaderOnReadCallback)
{
    ListPtr<daq::IPacket> packets;
    
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    signal.setDescriptor(createDataDescriptor());

    auto reader = PacketReader(signal);
    reader.setOnDataAvailable([&] {
        packets = reader.readAll();
        promise.set_value();
    });
    
    sendPacket(DataPacket(signal.getDescriptor(), 1, 1));
    scheduler.waitAll();

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(packets.getCount(), 2u);

    auto secondPacket = packets[1];
    ASSERT_EQ(secondPacket.getType(), PacketType::Data);

    auto dataPacket = secondPacket.asPtrOrNull<IDataPacket>(true);
    ASSERT_TRUE(dataPacket.assigned());
}

TEST_F(PacketReaderTest, PacketReaderFromPortOnReadCallback)
{
    ListPtr<daq::IPacket> packets;
    
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    signal.setDescriptor(createDataDescriptor());
    auto port = InputPort(signal.getContext(), nullptr, "readsig");

    auto reader = PacketReaderFromPort(port);
    port.connect(signal);

    reader.setOnDataAvailable([&] {
        packets = reader.readAll();
        promise.set_value();
    });
    
    sendPacket(DataPacket(signal.getDescriptor(), 1, 1));
    scheduler.waitAll();

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(packets.getCount(), 2u);

    auto secondPacket = packets[1];
    ASSERT_EQ(secondPacket.getType(), PacketType::Data);

    auto dataPacket = secondPacket.asPtrOrNull<IDataPacket>(true);
    ASSERT_TRUE(dataPacket.assigned());
}

TEST_F(PacketReaderTest, ReadingNotConnectedPort)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = daq::PacketReaderFromPort(port);

    auto availableCount = reader.getAvailableCount();
    ASSERT_EQ(availableCount, 0u);

    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 0u);

    // connecting port
    port.connect(this->signal);

    // check that event is encountered
    packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 1u);
    ASSERT_EQ(packets[0].getType(), PacketType::Event);
}

TEST_F(PacketReaderTest, NotifyPortIsConnected)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = daq::PacketReaderFromPort(port);

    ListPtr<IPacket> packets;
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    reader.setOnDataAvailable([&] {
        packets = reader.readAll();
        promise.set_value();
    });

    port.connect(this->signal);

    ASSERT_EQ(packets.getCount(), 1u);
    ASSERT_EQ(packets[0].getType(), PacketType::Event);
}