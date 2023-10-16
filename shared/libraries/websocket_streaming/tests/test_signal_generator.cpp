#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <signal_generator/signal_generator.h>
#include <chrono>

using namespace daq;

class SignalGeneratorTest : public testing::Test
{
public:
    ContextPtr context;
    SignalConfigPtr signal;
    SignalGenerator::GenerateSampleFunc stepFunction10;
    SignalGenerator::GenerateSampleFunc stepFunction100;

    void SetUp() override
    {
        context = NullContext();
        signal = createSignal();
        initFunctions();
    }

    void initFunctions()
    {
        stepFunction10 = [](uint64_t tick, void* sampleOut)
        {
            int* intOut = (int*) sampleOut;
            *intOut = tick % 10;
        };

        stepFunction100 = [](uint64_t tick, void* sampleOut)
        {
            int* intOut = (int*) sampleOut;
            *intOut = tick % 100;
        };
    }

    std::vector<int> calculateExpectedSamples(uint64_t startTick, size_t sampleCount, const SignalGenerator::GenerateSampleFunc& function)
    {
        auto samples = std::vector<int>(sampleCount);

        for (size_t i = 0; i < sampleCount; i++)
            function(startTick + i, samples.data() + i);

        return samples;
    }

    bool compareSamples(int* expected, void* packetData, size_t sampleCount)
    {
        return std::memcmp(expected, packetData, sampleCount * sizeof(int)) == 0;
    }

private:
    SignalConfigPtr createTimeSignal()
    {
        const size_t nanosecondsInSecond = 1000000000;
        auto delta = nanosecondsInSecond / 1000;

        auto descriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::UInt64)
                              .setRule(LinearDataRule(delta, 0))
                              .setTickResolution(Ratio(1, nanosecondsInSecond))
                              .setOrigin("1970-01-01T00:00:00")
                              .setName("Time")
                              .build();

        return SignalWithDescriptor(context, descriptor, nullptr, "Time");
    }

    SignalConfigPtr createSignal()
    {
        auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Int32).setName("Step").build();

        auto domainSignal = createTimeSignal();
        auto signal = SignalWithDescriptor(context, descriptor, nullptr, "ByteStep");
        signal.setDomainSignal(domainSignal);
        return signal;
    }
};


TEST_F(SignalGeneratorTest, CreateSignal)
{
    auto reader = PacketReader(signal);
    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 1u);
    ASSERT_EQ(packets[0].getType(), PacketType::Event);

    EventPacketPtr eventPacket = packets[0];
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
}

TEST_F(SignalGeneratorTest, StepSignal)
{
    const size_t packetSize = 100;

    auto expectedSamples1 = calculateExpectedSamples(0, packetSize, stepFunction10);
    auto expectedSamples2 = calculateExpectedSamples(packetSize, packetSize, stepFunction10);

    auto reader = PacketReader(signal);

    auto generator = SignalGenerator(signal);
    generator.setFunction(stepFunction10);
    generator.generateSamplesTo(std::chrono::milliseconds(packetSize));
    generator.generateSamplesTo(std::chrono::milliseconds(packetSize * 2));

    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 3u);

    auto packet1 = packets[1].asPtr<IDataPacket>();
    ASSERT_EQ(packet1.getSampleCount(), packetSize);
    ASSERT_TRUE(compareSamples(expectedSamples1.data(), packet1.getData(), packetSize));

    auto packet2 = packets[2].asPtr<IDataPacket>();
    ASSERT_EQ(packet2.getSampleCount(), packetSize);
    ASSERT_TRUE(compareSamples(expectedSamples2.data(), packet2.getData(), packetSize));
}

TEST_F(SignalGeneratorTest, ChangeFunction)
{
    const size_t packetSize = 100;

    auto expectedSamples1 = calculateExpectedSamples(0, packetSize, stepFunction10);
    auto expectedSamples2 = calculateExpectedSamples(packetSize, packetSize, stepFunction100);

    auto reader = PacketReader(signal);

    auto updateFunction = [this](SignalGenerator& generator, uint64_t packetOffset)
    {
        if (packetOffset > 0)
            generator.setFunction(stepFunction100);
    };

    auto generator = SignalGenerator(signal);
    generator.setFunction(stepFunction10);
    generator.setUpdateFunction(updateFunction);
    generator.generateSamplesTo(std::chrono::milliseconds(packetSize));
    generator.generateSamplesTo(std::chrono::milliseconds(packetSize * 2));

    auto packets = reader.readAll();
    ASSERT_EQ(packets.getCount(), 3u);

    auto packet1 = packets[1].asPtr<IDataPacket>();
    ASSERT_EQ(packet1.getSampleCount(), packetSize);
    ASSERT_TRUE(compareSamples(expectedSamples1.data(), packet1.getData(), packetSize));

    auto packet2 = packets[2].asPtr<IDataPacket>();
    ASSERT_EQ(packet2.getSampleCount(), packetSize);
    ASSERT_TRUE(compareSamples(expectedSamples2.data(), packet2.getData(), packetSize));
}
