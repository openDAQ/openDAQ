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
#include <opendaq/data_descriptor_factory.h>

#include <chrono>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

class PacketReaderTest : public testing::Test
{
public:
    explicit PacketReaderTest()
        : logger(Logger())
        , context(Context(Scheduler(logger, 1), logger, nullptr, nullptr))
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
