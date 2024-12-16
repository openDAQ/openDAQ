#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/reader_exceptions.h>
#include <opendaq/reader_factory.h>
#include <opendaq/time_reader.h>
#include "reader_common.h"

#include <gmock/gmock-matchers.h>

#include <chrono>
#include <future>
#include <thread>
#include <utility>

using namespace daq;
using namespace testing;

struct ReadSignal;

static void zeroOutPacketData(const DataPacketPtr& packet);
static DataPacketPtr createPacket(daq::SizeT numSamples, daq::Int offset, const ReadSignal& read);

struct ReadSignal
{
    explicit ReadSignal(const SignalConfigPtr& signal, Int packetOffset, Int packetSize)
        : packetSize(packetSize)
        , packetOffset(packetOffset)
        , signal(signal)
        , valueDescriptor(signal.getDescriptor())
    {
    }

    void setPacketOffset(Int offset)
    {
        packetOffset = offset;
    }

    void setPacketSize(Int size)
    {
        packetSize = size;
    }

    [[nodiscard]] SignalConfigPtr getDomainSignal() const
    {
        return signal.getDomainSignal();
    }

    [[nodiscard]] auto getDomainDescriptor() const
    {
        return getDomainSignal().getDescriptor();
    }

    void setValueDescriptor(const DataDescriptorPtr& descriptor)
    {
        signal.setDescriptor(descriptor);
        valueDescriptor = descriptor;
    }

    template <typename RoundTo = std::chrono::system_clock::duration>
    [[nodiscard]] auto toSysTime(ClockTick value, const DataDescriptorPtr& domainDataDescriptor = nullptr) const
    {
        using namespace std::chrono;

        auto dataDescriptor = domainDataDescriptor.assigned() ? domainDataDescriptor : getDomainDescriptor();

        system_clock::time_point parsedEpoch{};
        std::istringstream epochString(reader::fixupIso8601(dataDescriptor.getOrigin()));
        epochString >> date::parse("%FT%T%z", parsedEpoch);

        return reader::toSysTime<decltype(value), RoundTo>(value, parsedEpoch, dataDescriptor.getTickResolution());
    }

    template <typename ValueType = double>
    DataPacketPtr createAndSendPacket(Int packetIndex, bool log = false) const
    {
        Int delta = getDomainDescriptor().getRule().getParameters()["delta"];

        auto offset = packetOffset + ((packetSize * delta) * packetIndex);
        if (log)
        {
            std::cout << "<" << packetIndex << "> " << "(off: " << offset << " pSize: " << packetSize << " pOffset: " << packetOffset << ")"
                      << std::endl;
        }

        auto packet = createPacket(packetSize, offset, *this);
        zeroOutPacketData(packet);

        auto* data = static_cast<ValueType*>(packet.getData());
        for (auto i = 0; i < packetSize; ++i)
        {
            data[i] = offset + i;
        }

        if (log)
        {
            ClockTick* ticks = static_cast<ClockTick*>(packet.getDomainPacket().getData());
            for (int i = 0; i < packetSize; ++i)
            {
                std::cout << ticks[i] << std::endl;
            }
        }

        signal.sendPacket(packet);
        return packet;
    }

    Int packetSize;
    Int packetOffset;
    SignalConfigPtr signal;
    DataDescriptorPtr valueDescriptor;
};

static std::string loggerEnvVariable{"OPENDAQ_SINK_WINDEBUG_LOG_LEVEL"};

class MultiReaderTest : public ReaderTest<>
{
public:
    using Super = ReaderTest<>;

    ReadSignal& addSignal(Int packetOffset, Int packetSize, const SignalPtr& domain, SampleType valueType = SampleType::Float64)
    {
        auto newSignal = Signal(context, nullptr, fmt::format("sig{}", counter++));
        newSignal.setDescriptor(setupDescriptor(valueType));
        newSignal.setDomainSignal(domain);

        return readSignals.emplace_back(newSignal, packetOffset, packetSize);
    }

    [[nodiscard]] SignalConfigPtr createDomainSignal(std::string epoch = "",
                                                     const daq::RatioPtr& resolution = nullptr,
                                                     const daq::DataRulePtr& rule = nullptr,
                                                     const daq::ReferenceDomainInfoPtr& referenceDomainInfo = nullptr) const
    {
        auto domain = Signal(context, nullptr, "time");
        domain.setDescriptor(createDomainDescriptor(std::move(epoch), resolution, rule, referenceDomainInfo));

        return domain;
    }

    void sendPackets(Int index) const
    {
        for (const auto& signal : readSignals)
        {
            signal.createAndSendPacket(index);
        }
    }

    [[nodiscard]] ListPtr<ISignal> signalsToList() const
    {
        ListPtr<SignalConfigPtr> signals = List<ISignalConfig>();
        for (const auto& read : readSignals)
        {
            signals.pushBack(read.signal);
        }
        return signals;
    }

    [[nodiscard]] ListPtr<IInputPortConfig> portsList(bool enableGapDetection = false) const
    {
        ListPtr<IInputPortConfig> ports = List<IInputPortConfig>();
        size_t index = 0;
        for (const auto& read : readSignals)
        {
            std::string localId = "readsig" + std::to_string(index++);
            auto port = InputPort(read.signal.getContext(), nullptr, localId, enableGapDetection);
            ports.pushBack(port);
        }
        return ports;
    }

    template <typename RoundTimeTo, typename T, typename U, typename V>
    void printData(std::int64_t samples, T& times, U& values, V& domain) const
    {
        using namespace reader;

        int numSignals = static_cast<int>(readSignals.size());
        for (int sigIndex = 0; sigIndex < numSignals; ++sigIndex)
        {
            fmt::print("--------\n");
            fmt::print("Signal {}\n", sigIndex);

            for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
            {
                times[sigIndex][sampleIndex] = readSignals[sigIndex].toSysTime<RoundTimeTo>(domain[sigIndex][sampleIndex]);
                std::cout << times[sigIndex][sampleIndex];

                fmt::print(" |d: {} |v: {}\n", domain[sigIndex][sampleIndex], values[sigIndex][sampleIndex]);
            }
        }
    }

    template <typename T, typename U>
    void printData(std::int64_t samples, T& times, U& values) const
    {
        using namespace std::chrono;
        using namespace reader;

        int numSignals = static_cast<int>(readSignals.size());
        for (int sigIndex = 0; sigIndex < numSignals; ++sigIndex)
        {
            fmt::print("--------\n");
            fmt::print("Signal {}\n", sigIndex);

            for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
            {
                std::stringstream ss;
                ss << times[sigIndex][sampleIndex];

                fmt::print(" |d: {} |v: {}\n", ss.str(), values[sigIndex][sampleIndex]);
            }
        }
    }

    template <typename RoundTimeTo, typename T>
    void roundData(std::int64_t samples, T& times)
    {
        int numSignals = static_cast<int>(readSignals.size());
        for (int sigIndex = 0; sigIndex < numSignals; ++sigIndex)
        {
            for (int sampleIndex = 0; sampleIndex < samples; ++sampleIndex)
            {
                times[sigIndex][sampleIndex] = std::chrono::round<RoundTimeTo>(times[sigIndex][sampleIndex]);
            }
        }
    }

protected:
    int counter{};
    LogLevel prevLogLevel{LogLevel::Default};
    SignalConfigPtr domainSignal;
    std::vector<ReadSignal> readSignals;
};

static void zeroOutPacketData(const DataPacketPtr& packet)
{
    memset(packet.getRawData(), 0, packet.getRawDataSize());
}

static DataPacketPtr createPacket(daq::SizeT numSamples, daq::Int offset, const ReadSignal& read)
{
    auto domainPacket = daq::DataPacket(read.getDomainDescriptor(), numSamples, offset);

    return daq::DataPacketWithDomain(domainPacket, read.valueDescriptor, numSamples);
}

TEST_F(MultiReaderTest, SignalStartDomainFrom0)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, SignalStartDomainFrom0SkipSamples)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    SizeT skip = 100;
    multi.skipSamples(&skip);
    ASSERT_EQ(skip, 100u);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 346u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
    ASSERT_EQ(domain[0][0], 1223);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 341u);

    skip = 1000;
    multi.skipSamples(&skip);
    ASSERT_EQ(skip, 341u);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);
}

TEST_F(MultiReaderTest, IsSynchronized)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());
    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    ASSERT_FALSE(multi.getIsSynchronized());

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    ASSERT_FALSE(multi.getIsSynchronized());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    ASSERT_FALSE(multi.getIsSynchronized());

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    ASSERT_TRUE(multi.getIsSynchronized());

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 441u);

    ASSERT_TRUE(multi.getIsSynchronized());
}

TEST_F(MultiReaderTest, SignalStartDomainFrom0Raw)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"), SampleType::Float64);
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"), SampleType::Int64);
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"), SampleType::UInt32);

    auto multi = MultiReaderRaw(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket<double>(0);
    sig1.createAndSendPacket<int64_t>(0);
    sig2.createAndSendPacket<uint32_t>(0);

    sig0.createAndSendPacket<double>(1);
    sig1.createAndSendPacket<int64_t>(1);
    sig2.createAndSendPacket<uint32_t>(1);

    sig0.createAndSendPacket<double>(2);
    sig1.createAndSendPacket<int64_t>(2);
    sig2.createAndSendPacket<uint32_t>(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = 5u;

    double values0[SAMPLES]{};
    std::int64_t values1[SAMPLES]{};
    std::uint32_t values2[SAMPLES]{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values0, values1, values2};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    // printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, SignalStartRelativeOffset0)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG0_PACKET_SIZE = 523u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, SIG0_PACKET_SIZE, createDomainSignal(" "));
    auto& sig1 = addSignal(0, 732, createDomainSignal(" "));
    auto& sig2 = addSignal(0, 843, createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, SIG0_PACKET_SIZE * 3);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, SignalStartDomainFrom0Timeout)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG1_PACKET_SIZE = 523;

    // prevent vector from re-allocating so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, SIG1_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = SIG1_PACKET_SIZE + 1;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    std::thread thread(
        [sig0, sig1, sig2]
        {
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(200ms);

            sig0.createAndSendPacket(3);
            sig1.createAndSendPacket(3);
            sig2.createAndSendPacket(3);
        });

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count, 1000u);

    if (thread.joinable())
        thread.join();

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, SignalStartDomainFrom0TimeoutExceeded)
{
    SKIP_TEST_MAC_CI;

    using namespace std::chrono;
    using namespace std::chrono_literals;

    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG1_PACKET_SIZE = 523;

    // prevent vector from re-allocating so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, SIG1_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = SIG1_PACKET_SIZE * 2;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    std::thread thread(
        [sig0, sig1, sig2]
        {
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(200ms);

            sig0.createAndSendPacket(3);
            sig1.createAndSendPacket(3);
            sig2.createAndSendPacket(3);
        });

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count, 300);

    if (thread.joinable())
        thread.join();

    ASSERT_EQ(count, SAMPLES - 77);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(static_cast<std::int64_t>(count), time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, WithPacketOffsetNot0)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 458u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, WithPacketOffsetNot0Relative)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG0_PACKET_SIZE = 523u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, SIG0_PACKET_SIZE, createDomainSignal(" "));
    auto& sig1 = addSignal(134, 732, createDomainSignal(" "));
    auto& sig2 = addSignal(111, 843, createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    // samples needed to sync sig0 (134 - 123 = 11)
    ASSERT_EQ(available, (SIG0_PACKET_SIZE * 3) - 11);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, MaxTimeIsNotOnSignalWithMaxEpoch)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG0_PACKET_SIZE = 523u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(1240, SIG0_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:03+00:00"));  // 03:4.24
    auto& sig1 = addSignal(134, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, SIG0_PACKET_SIZE * 3);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, Clock10kHzDelta10)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, Clock10kHzDelta10Relative)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG0_PACKET_SIZE = 523u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, SIG0_PACKET_SIZE, createDomainSignal(" "));
    auto& sig1 = addSignal(0, 732, createDomainSignal(" ", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, SIG0_PACKET_SIZE * 3);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, Clock10kHzDelta10WithAlignedOffset)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(1240, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(130, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 1569u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, Clock10kHzDelta10WithAlignedOffsetRelative)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(1240, 523, createDomainSignal(" "));
    auto& sig1 = addSignal(130, 732, createDomainSignal(" ", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(111, 843, createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 969u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, Clock10kHzDelta10WithIntersampleOffset)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG10_PACKET_SIZE = 523;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(1240, SIG10_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(131, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 3u * SIG10_PACKET_SIZE);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    std::array readDomain{time[0][0], time[1][0], time[2][0]};

    using Seconds = std::chrono::duration<double>;

    auto firstTime = time[0][0];
    ASSERT_THAT(readDomain, testing::ElementsAre(firstTime, firstTime + Seconds(0.0001), firstTime));
}

TEST_F(MultiReaderTest, EpochChanged)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG1_PACKET_SIZE = 732;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, SIG1_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());
    TimeReader timeReader(multi);

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig1.getDomainSignal().setDescriptor(createDomainDescriptor("2022-09-27T00:02:04.1+00:00"));

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    sig0.createAndSendPacket(3);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 632u);

    // Read over the signal-descriptor change (it will stop on event so maximim read samples will be still 632)
    constexpr const SizeT SAMPLES = SIG1_PACKET_SIZE + 1;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{time[0], time[1], time[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_EQ(count, available);

    printData(count, time, values);
    roundData<std::chrono::microseconds>(count, time);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, EpochChangedBeforeFirstData)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig1.getDomainSignal().setDescriptor(createDomainDescriptor("2022-09-27T00:02:04.1+00:00"));

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{0};
    MultiReaderStatusPtr status = multi.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_TRUE(status.getEventPackets().assigned());
    ASSERT_EQ(status.getEventPackets().getCount(), 1u);
    ASSERT_TRUE(status.getEventPackets().hasKey("/multi_reader_signal_sig1"));
    ASSERT_NE(status.getEventPackets().get("/multi_reader_signal_sig1"), nullptr);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 458u);

    count = SAMPLES;
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, Signal2Invalidated)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG2_PACKET_SIZE = 843u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, SIG2_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());
    TimeReader timeReader(multi);

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig2.setValueDescriptor(setupDescriptor(SampleType::ComplexFloat64));

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    sig0.createAndSendPacket(3);

    available = multi.getAvailableCount();

    // 1 packet available until descriptor changes
    ASSERT_EQ(available, SIG2_PACKET_SIZE);

    // Read over the signal-descriptor change
    constexpr const SizeT SAMPLES = SIG2_PACKET_SIZE + 1;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{time[0], time[1], time[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_EQ(count, SIG2_PACKET_SIZE);

    auto status = multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_FALSE(status.getValid());

    printData(SAMPLES, time, values);
    roundData<std::chrono::microseconds>(SAMPLES, time);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, ResolutionChanged)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG1_PACKET_SIZE = 732;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, SIG1_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());
    TimeReader timeReader(multi);

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0, false);
    sig2.createAndSendPacket(0);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    // Increase the resolution at the same sample-rate
    sig1.getDomainSignal().setDescriptor(createDomainDescriptor("2022-09-27T00:02:04+00:00", Ratio(1, 10000), LinearDataRule(10, 0)));
    sig1.packetOffset *= 10;

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1, false);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2, false);
    sig2.createAndSendPacket(2);

    sig0.createAndSendPacket(3);
    sig1.createAndSendPacket(3, false);
    sig2.createAndSendPacket(3);

    available = multi.getAvailableCount();
    // 732 - 100 needed to sync before descriptor changed
    ASSERT_EQ(available, 632u);

    // Read over the signal-descriptor change. it will stops on event. so it will read 632 as getAvailableCount return synced samples until
    // event
    constexpr const SizeT SAMPLES = SIG1_PACKET_SIZE + 1;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{time[0], time[1], time[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_EQ(count, available);

    printData(available, time, values);
    roundData<std::chrono::microseconds>(available, time);

    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, SampleRateChanged)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG1_PACKET_SIZE = 732u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, SIG1_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());
    TimeReader timeReader(multi);

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0, false);
    sig2.createAndSendPacket(0);

    // Change the sample-rate (same resolution, different delta)
    sig1.getDomainSignal().setDescriptor(createDomainDescriptor("2022-09-27T00:02:04+00:00", nullptr, LinearDataRule(10, 0)));

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1, false);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2, false);
    sig2.createAndSendPacket(2);

    sig0.createAndSendPacket(3);
    sig1.createAndSendPacket(3, false);
    sig2.createAndSendPacket(3);

    available = multi.getAvailableCount();

    // 732 - 100 needed to sync until next descriptor change
    ASSERT_EQ(available, SIG1_PACKET_SIZE - 100);

    // Read over the signal-descriptor change
    constexpr const SizeT SAMPLES = SIG1_PACKET_SIZE + 1;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{time[0], time[1], time[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_EQ(count, 632u);

    auto status = multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_FALSE(status.getValid());

    printData(SAMPLES, time, values);
    roundData<std::chrono::microseconds>(SAMPLES, time);

    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, ReuseReader)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const auto SIG2_PACKET_SIZE = 843u;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(123, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(134, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(111, SIG2_PACKET_SIZE, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    std::array<ComplexFloat64, NUM_SIGNALS> oldReaderNextValues{};

    auto multi = MultiReader(signalsToList());
    {
        TimeReader timeReader(multi);

        {
            SizeT count{0};
            auto status = multi.read(nullptr, &count);
            ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
        }

        auto available = multi.getAvailableCount();
        ASSERT_EQ(available, 0u);

        sig0.createAndSendPacket(0);
        sig1.createAndSendPacket(0);
        sig2.createAndSendPacket(0);

        sig2.setValueDescriptor(setupDescriptor(SampleType::ComplexFloat64));

        sig0.createAndSendPacket(1);
        sig1.createAndSendPacket(1);
        sig2.createAndSendPacket<ComplexFloat64>(1);

        sig0.createAndSendPacket(2);
        sig1.createAndSendPacket(2);
        sig2.createAndSendPacket<ComplexFloat64>(2);

        sig0.createAndSendPacket(3);

        available = multi.getAvailableCount();
        // 843 - 0 needed to synchronize until next descriptor
        ASSERT_EQ(available, SIG2_PACKET_SIZE);

        // Read over the signal-descriptor change
        constexpr const SizeT SAMPLES = SIG2_PACKET_SIZE + 1;

        std::array<double[SAMPLES], NUM_SIGNALS> values{};
        std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};

        void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
        void* domainPerSignal[NUM_SIGNALS]{time[0], time[1], time[2]};

        SizeT count{SAMPLES};
        multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
        ASSERT_EQ(count, SIG2_PACKET_SIZE);

        auto status = multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
        ASSERT_FALSE(status.getValid());

        roundData<std::chrono::microseconds>(SAMPLES, time);
        ASSERT_THAT(time[1], ElementsAreArray(time[0]));
        ASSERT_THAT(time[2], ElementsAreArray(time[0]));

        for (int i = 0; i < NUM_SIGNALS; ++i)
        {
            oldReaderNextValues[i] = values[i][SIG2_PACKET_SIZE - 1] + 1;
        }
    }

    auto reused = MultiReaderFromExisting<ComplexFloat64>(multi);

    std::array<ComplexFloat64, NUM_SIGNALS> values{};
    void* valuesPerSignal[NUM_SIGNALS]{&values[0], &values[1], &values[2]};

    SizeT samples{1u};
    reused.read(valuesPerSignal, &samples);

    ASSERT_EQ(samples, 1u);
    ASSERT_THAT(values, ElementsAreArray(oldReaderNextValues));
}

TEST_F(MultiReaderTest, MultiReaderWithInputPort)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto ports = portsList();
    auto signals = signalsToList();
    auto multi = MultiReaderFromPort(ports);
    for (size_t i = 0; i < NUM_SIGNALS; i++)
        ports[i].connect(signals[i]);

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    auto status = multi.read(nullptr, &available);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, MultiReaderWithNotConnectedInputPort)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto portList = List<IInputPortConfig>();
    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList.pushBack(InputPort(readSignals[i].signal.getContext(), nullptr, "readsig" + std::to_string(i)));

    auto ports = portsList();
    auto signals = signalsToList();
    auto multi = MultiReaderFromPort(ports);

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    for (size_t i = 0; i < NUM_SIGNALS; i++)
        ports[i].connect(signals[i]);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    auto status = multi.read(nullptr, &available);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 446u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, MultiReaderWithDifferentInputs)
{
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto portList = portsList();
    auto componentList = List<IComponent>(portList[0], portList[1], sig2.signal);

    ASSERT_THROW(MultiReaderFromPort(componentList), InvalidParameterException);
}

TEST_F(MultiReaderTest, MultipleMultiReaderToInputPort)
{
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(1);

    addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();

    auto reader1 = MultiReaderFromPort(portList);
    ASSERT_THROW(MultiReaderFromPort(portList), AlreadyExistsException);
}

TEST_F(MultiReaderTest, MultiReaderReuseInputPort)
{
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(1);

    addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();

    {
        auto reader1 = MultiReaderFromPort(portList);
    }
    ASSERT_NO_THROW(MultiReaderFromPort(portList));
}

TEST_F(MultiReaderTest, MultiReaderOnReadCallback)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    SizeT count{SAMPLES};
    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto reader = MultiReader(signalsToList());

    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    reader.setOnDataAvailable(
        [&, promise = &promise]() mutable
        {
            if (reader.getAvailableCount() < count)
                return;

            reader.readWithDomain(valuesPerSignal, domainPerSignal, &count);
            reader.setOnDataAvailable(nullptr);  // trigger callback only once
            promise->set_value();
        });

    auto available = reader.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, MultiReaderFromPortOnReadCallback)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    SizeT count{SAMPLES};
    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto ports = portsList();
    auto signals = signalsToList();
    auto reader = MultiReaderFromPort(ports);
    for (size_t i = 0; i < NUM_SIGNALS; i++)
        ports[i].connect(signals[i]);

    SizeT toRead = 0u;
    auto status = reader.read(nullptr, &toRead);

    reader.setOnDataAvailable(
        [&, promise = &promise]
        {
            if (reader.getAvailableCount() < count)
                return;

            reader.readWithDomain(valuesPerSignal, domainPerSignal, &count);
            reader.setOnDataAvailable(nullptr);  // trigger callback only once
            promise->set_value();
        });

    auto available = reader.getAvailableCount();
    ASSERT_EQ(available, 0u);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, StartOnFullUnitOfDomain)
{
    constexpr const auto NUM_SIGNALS = 3;

    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReaderEx(signalsToList(), ReadTimeoutType::All, -1, true);

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    for (Int i = 0; i < 5; i++)
    {
        sig0.createAndSendPacket(i);
        sig1.createAndSendPacket(i);
        sig2.createAndSendPacket(i);
    }

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 615u);

    constexpr const SizeT SAMPLES = 5u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    ASSERT_THAT(time[1], ElementsAreArray(time[0]));
    ASSERT_THAT(time[2], ElementsAreArray(time[0]));
}

TEST_F(MultiReaderTest, SampleRateDivider)
{
    constexpr const auto NUM_SIGNALS = 3;
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    std::array<std::int32_t, NUM_SIGNALS> dividers = {1, 2, 5};

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00", nullptr, LinearDataRule(dividers[0], 0)));  // 1000 Hz
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", nullptr, LinearDataRule(dividers[1], 0)));  // 500 Hz
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.125+00:00", nullptr, LinearDataRule(dividers[2], 0)));  // 200 Hz

    auto multi = MultiReader(signalsToList());

    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    ASSERT_EQ(multi.getCommonSampleRate(), 1000);

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    for (Int i = 0; i < 5; i++)
    {
        sig0.createAndSendPacket(i);
        sig1.createAndSendPacket(i);
        sig2.createAndSendPacket(i);
    }

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 1480u);

    constexpr const SizeT SAMPLES = 52u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES - 2);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    std::array<ClockTick, NUM_SIGNALS> lastTimeStamp;

    for (SizeT i = 0; i < dividers.size(); i++)
    {
        const SizeT numberOfSamples = count / dividers[i];

        ASSERT_EQ(time[i][0], time[0][0]);
        lastTimeStamp[i] = domain[i][numberOfSamples - 1];

        for (SizeT j = 1; j < numberOfSamples; j++)
            ASSERT_EQ(domain[i][j] - domain[i][j - 1], dividers[i]);
    }

    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    for (SizeT i = 0; i < dividers.size(); i++)
    {
        ASSERT_EQ(domain[i][0] - lastTimeStamp[i], dividers[i]);
    }
}

TEST_F(MultiReaderTest, SampleRateDividerRequiredRate)
{
    constexpr const auto NUM_SIGNALS = 3;
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(3);

    std::array<std::int32_t, NUM_SIGNALS> dividers = {1, 2, 5};
    const std::int64_t reqiredRate = 2000;

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00", nullptr, LinearDataRule(dividers[0], 0)));  // 1000 Hz
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", nullptr, LinearDataRule(dividers[1], 0)));  // 500 Hz
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.125+00:00", nullptr, LinearDataRule(dividers[2], 0)));  // 200 Hz

    auto multi =
        MultiReaderEx(signalsToList(), SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All, reqiredRate, false);
    {
        SizeT count{0};
        auto status = multi.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    ASSERT_EQ(multi.getCommonSampleRate(), reqiredRate);

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    for (Int i = 0; i < 5; i++)
    {
        sig0.createAndSendPacket(i);
        sig1.createAndSendPacket(i);
        sig2.createAndSendPacket(i);
    }

    available = multi.getAvailableCount();
    ASSERT_EQ(available, 2960u);

    constexpr const SizeT SAMPLES = 102u;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    ASSERT_EQ(count, SAMPLES - 2);

    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> time{};
    printData<std::chrono::microseconds>(SAMPLES, time, values, domain);

    std::array<ClockTick, NUM_SIGNALS> lastTimeStamp;

    for (SizeT i = 0; i < dividers.size(); i++)
    {
        const SizeT numberOfSamples = count / dividers[i] / 2;

        ASSERT_EQ(time[i][0], time[0][0]);
        lastTimeStamp[i] = domain[i][numberOfSamples - 1];

        for (SizeT j = 1; j < numberOfSamples; j++)
            ASSERT_EQ(domain[i][j] - domain[i][j - 1], dividers[i]);
    }

    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    for (SizeT i = 0; i < dividers.size(); i++)
    {
        ASSERT_EQ(domain[i][0] - lastTimeStamp[i], dividers[i]);
    }
}

TEST_F(MultiReaderTest, MultiReaderBuilderGetSet)
{
    SignalPtr sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00")).signal;
    SignalPtr sig1 =
        addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0))).signal;
    SignalPtr sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00")).signal;

    auto portList = portsList();

    auto builder = MultiReaderBuilder();
    builder.addInputPort(portList[0]);
    builder.addSignal(sig1);
    builder.addSignal(sig2);
    builder.setValueReadType(SampleType::Int16);
    builder.setDomainReadType(SampleType::Int16);
    builder.setReadMode(ReadMode::RawValue);
    builder.setReadTimeoutType(ReadTimeoutType::Any);
    builder.setRequiredCommonSampleRate(0);
    builder.setStartOnFullUnitOfDomain(true);

    ASSERT_EQ(builder.getSourceComponents().getCount(), 3u);
    ASSERT_EQ(builder.getSourceComponents()[0].asPtr<IInputPort>().getSignal(), nullptr);
    ASSERT_EQ(builder.getSourceComponents()[1], sig1);
    ASSERT_EQ(builder.getSourceComponents()[2], sig2);

    ASSERT_EQ(builder.getValueReadType(), SampleType::Int16);
    ASSERT_EQ(builder.getDomainReadType(), SampleType::Int16);
    ASSERT_EQ(builder.getReadMode(), ReadMode::RawValue);
    ASSERT_EQ(builder.getReadTimeoutType(), ReadTimeoutType::Any);
    ASSERT_EQ(builder.getRequiredCommonSampleRate(), 0);
    ASSERT_EQ(builder.getStartOnFullUnitOfDomain(), true);
}

TEST_F(MultiReaderTest, MultiReaderBuilderWithDifferentInputs)
{
    readSignals.reserve(3);

    auto sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto portList = portsList();

    SignalPtr signal1 = sig1.signal;
    SignalPtr signal2 = sig2.signal;

    MultiReaderBuilderPtr builder = MultiReaderBuilder().addInputPort(portList[0]).addSignal(signal1).addSignal(signal2);
    ASSERT_THROW(MultiReaderFromBuilder(builder), InvalidParameterException);
}

TEST_F(MultiReaderTest, MultiReaderBuilderFromSignalsTimeouts)
{
    using namespace std::chrono_literals;
    readSignals.reserve(3);

    auto sig0 = addSignal(0, 10, createDomainSignal());
    auto sig1 = addSignal(0, 10, createDomainSignal());
    auto sig2 = addSignal(0, 10, createDomainSignal());

    SignalPtr signal0 = sig0.signal;
    SignalPtr signal1 = sig1.signal;
    SignalPtr signal2 = sig2.signal;

    MultiReaderBuilderPtr builder = MultiReaderBuilder().addSignal(signal0).addSignal(signal1).addSignal(signal2);
    auto multireader = builder.build();

    using Type = SampleTypeToType<SampleType::Float64>::Type;
    Type sig0Samples[10];
    Type sig1Samples[10];
    Type sig2Samples[10];

    void* samples[3] = {sig0Samples, sig1Samples, sig2Samples};
    auto count = SizeT{0};
    auto status = multireader.read(samples, &count, 1000);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    auto start = std::chrono::steady_clock::now();
    auto sendThread = std::thread([&sig0, &sig1, &sig2]() {
        std::this_thread::sleep_for(1s);
        sig0.createAndSendPacket(0);
        sig1.createAndSendPacket(0);
        sig2.createAndSendPacket(0);
    });
    count = SizeT(sig0.packetSize);
    status = multireader.read(samples, &count, 10000);
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 10);
    ASSERT_LT(diff, 5000);

    sendThread.join();
}

TEST_F(MultiReaderTest, MultiReaderTimeoutWhenDataAvailable)
{
    using namespace std::chrono_literals;
    readSignals.reserve(3);

    auto sig0 = addSignal(0, 10, createDomainSignal());
    auto sig1 = addSignal(0, 10, createDomainSignal());
    auto sig2 = addSignal(0, 10, createDomainSignal());

    auto multireader = MultiReader(signalsToList());

    using Type = SampleTypeToType<SampleType::Float64>::Type;
    Type sig0Samples[10];
    Type sig1Samples[10];
    Type sig2Samples[10];

    void* samples[3] = {sig0Samples, sig1Samples, sig2Samples};
    auto count = SizeT{0};
    auto start = std::chrono::steady_clock::now();
    auto status = multireader.read(samples, &count, 10000);
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_LT(diff, 5000);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    ASSERT_EQ(multireader.getAvailableCount(), 10);

    count = SizeT(sig0.packetSize);
    start = std::chrono::steady_clock::now();
    status = multireader.read(samples, &count, 10000);
    end = std::chrono::steady_clock::now();
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Diff = " << diff << std::endl;
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 10);
    ASSERT_LT(diff, 5000);
}

TEST_F(MultiReaderTest, MultiReaderExceptionOnConstructor)
{
    readSignals.reserve(1);

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00", nullptr, LinearDataRule(1, 0)));  // 1000 Hz

    MultiReaderPtr mr;
    try
    {
        mr = MultiReaderEx(signalsToList(), SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All, 500, false);
    }
    catch (...)
    {
    }

    for (Int i = 0; i < 5; i++)
    {
        sig0.createAndSendPacket(i);
    }
}

TEST_F(MultiReaderTest, MultiReaderTimeoutChecking)
{
    readSignals.reserve(2);

    auto sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:03+00:00"));

    const MultiReaderPtr multiReader = MultiReader(signalsToList(), SampleType::Float64, SampleType::Int64);

    {
        SizeT count{0};
        auto status = multiReader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    constexpr size_t numberOfSamplesToRead = 8;
    double dataFirstSignal[numberOfSamplesToRead];
    double dataSecondSignal[numberOfSamplesToRead];
    double* data[2]{dataFirstSignal, dataSecondSignal};

    size_t count = numberOfSamplesToRead;
    auto a1 = std::async(std::launch::async, [&] { multiReader.read(data, &count, 10000); });

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    a1.wait();

    ASSERT_EQ(count, numberOfSamplesToRead);
}

TEST_F(MultiReaderTest, DISABLED_MultiReaderGapDetection)
{
    constexpr const auto NUM_SIGNALS = 2;
    readSignals.reserve(NUM_SIGNALS);

    // time different between the two signals is 0.01s which is 10 samples
    auto& sig0 = addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00", nullptr, LinearDataRule(1, 0)));
    auto& sig1 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03.01+00:00", nullptr, LinearDataRule(1, 0)));

    auto ports = portsList(true);
    auto signals = signalsToList();
    auto multi = MultiReader(ports);
    for (size_t i = 0; i < NUM_SIGNALS; i++)
        ports[i].connect(signals[i]);

    SizeT toRead = 0u;
    MultiReaderStatusPtr status = multi.read(nullptr, &toRead);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    // for signal 0 writes first packet with 10 samples, and then second packet with 10 samples and offset of 5
    // in signal will be generated 3 packets
    // data packet with 10 samples
    // event packet with gap in 5 samples
    // data packet with 10 samples
    sig0.createAndSendPacket(0);
    sig0.packetOffset = 5;
    sig0.createAndSendPacket(1);

    // for signal 1 - write 20 samples
    // in signal will be generated 1 packet with 20 samples
    sig1.createAndSendPacket(0);
    sig1.signal.getContext().getScheduler().waitAll();

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    SizeT count{0};
    status = multi.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_TRUE(status.getEventPackets().assigned());
    ASSERT_EQ(status.getEventPackets().getCount(), 1);
    ASSERT_TRUE(status.getEventPackets().hasKey("/readsig0"));

    auto event = status.getEventPackets().get("/readsig0");
    ASSERT_EQ(event.getEventId(), event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED);
    ASSERT_EQ(event.getParameters().get(event_packet_param::GAP_DIFF), 5);

    constexpr const SizeT SAMPLES = 10;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1]};

    count = SAMPLES;
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);

    // bacause the was no shifting in time
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReaderTest, ReadWhenOnePortIsNotConnected)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 30, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 40, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();
    auto multi = MultiReaderFromPort(portList);
    portList[0].connect(sig0.signal);
    portList[1].connect(sig1.signal);

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    constexpr const SizeT SAMPLES = 10u;
    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};

    // check that we read 0 samples as one of the ports is not connected
    SizeT count{SAMPLES};
    MultiReaderStatusPtr status = multi.read(valuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 0u);

    // check reading with timeout
    count = SAMPLES;
    status = multi.read(valuesPerSignal, &count, 100u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 0u);

    // connect signal to the port
    portList[2].connect(sig2.signal);
    sig2.signal.getContext().getScheduler().waitAll();
    sig2.createAndSendPacket(0);

    // first packet on sig2 will be the event packet
    count = 0;
    status = multi.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getEventPackets().getCount(), 3u);

    count = SAMPLES;
    status = multi.read(valuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReaderTest, NotifyPortIsConnected)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 30, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 40, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();
    auto multi = MultiReaderFromPort(portList);
    portList[0].connect(sig0.signal);
    portList[1].connect(sig1.signal);

    MultiReaderStatusPtr status;

    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    multi.setOnDataAvailable(
        [&]
        {
            SizeT count{0};
            status = multi.read(nullptr, &count);
            promise.set_value();
        });

    portList[2].connect(sig2.signal);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getEventPackets().getCount(), 3u);
}

TEST_F(MultiReaderTest, ReadWhilePortIsNotConnected)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 30, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 40, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();
    auto multi = MultiReaderFromPort(portList);
    portList[0].connect(sig0.signal);
    portList[1].connect(sig1.signal);

    MultiReaderStatusPtr status;

    std::future<void> future = std::async(std::launch::async,
                                          [&]
                                          {
                                              SizeT count{0};
                                              status = multi.read(nullptr, &count, 1000u);
                                          });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    portList[2].connect(sig2.signal);

    future.wait();
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getEventPackets().getCount(), 3u);
}

TEST_F_UNSTABLE_SKIPPED(MultiReaderTest, ReconnectWhileReading)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 30, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();
    auto multi = MultiReaderFromPort(portList);
    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList[i].connect(readSignals[i].signal);

    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    constexpr const SizeT SAMPLES = 20u;
    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};

    SizeT count{SAMPLES};
    MultiReaderStatusPtr status;
    std::future<void> future = std::async(std::launch::async,
                                          [&]
                                          {
                                              SizeT tmpCnt{0};
                                              status = multi.read(nullptr, &tmpCnt, 500);
                                              multi.read(valuesPerSignal, &count, 300);
                                          });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sig0.createAndSendPacket(0);
    sig0.signal.getContext().getScheduler().waitAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    portList[0].disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    portList[0].connect(sig0.signal);

    future.wait();
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getEventPackets().getCount(), 3u);
    ASSERT_TRUE(status.getEventPackets().hasKey("/readsig0"));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEquality01)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993"));
    addSignal(0, 113, createDomainSignal("1993"));
    addSignal(0, 133, createDomainSignal("1993"));

    ASSERT_NO_THROW(MultiReader(signalsToList()));

#if !defined(_DEBUG)
    return;
#endif

    auto received = privateSink.waitForMessage(9001);
    ASSERT_TRUE(received);
    auto str = privateSink.getLastMessage();
    ASSERT_EQ(str, R"(Domain signal "time" Reference Domain Info is not assigned.)");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEquality02)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 133, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEquality03)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 133, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}
TEST_F(MultiReaderTest, ReferenceDomainIdEquality04)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).build()));
    addSignal(0, 133, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEquality05)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 133, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequality01)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("B").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequality02)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("B").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequality03)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("B").build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequality04)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("B").build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequality05)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("B").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequality06)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("B").build()));
    addSignal(0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceDomainId("A").build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceEquality01)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0, 113, createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));

#if !defined(_DEBUG)
    return;
#endif

    auto received = privateSink.waitForMessage(9001);
    ASSERT_TRUE(received);
    auto str = privateSink.getLastMessage();
    ASSERT_EQ(str, R"(Domain signal "time" Reference Domain ID not assigned.)");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceEquality02)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993", nullptr, nullptr, ReferenceDomainInfoBuilder().setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));

#if !defined(_DEBUG)
    return;
#endif

    auto received = privateSink.waitForMessage(9001);
    ASSERT_TRUE(received);
    auto str = privateSink.getLastMessage();
    ASSERT_EQ(str, R"(Domain signal "time" Reference Time Source is Unknown.)");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceEquality03)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceEquality04)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality01)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality02)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality03)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality04)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality05)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality06)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality07)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality08)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality09)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality10)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality11)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality12)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality13)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality01)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality02)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality03)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality04)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality05)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality06)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_THROW_MSG(MultiReader(signalsToList()), InvalidStateException, "Reference domain is incompatible.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality07)
{
    constexpr const auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Gps).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality08)
{
    constexpr const auto NUM_SIGNALS = 4;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality09)
{
    constexpr const auto NUM_SIGNALS = 4;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality10)
{
    constexpr const auto NUM_SIGNALS = 6;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality11)
{
    constexpr const auto NUM_SIGNALS = 6;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Gps).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality12)
{
    constexpr const auto NUM_SIGNALS = 12;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality13)
{
    constexpr const auto NUM_SIGNALS = 12;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_NO_THROW(MultiReader(signalsToList()));
}

TEST_F(MultiReaderTest, ReferenceDomainIdInequalityReferenceTimeSourceInequality14)
{
    constexpr const auto NUM_SIGNALS = 12;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("B").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId(nullptr).setReferenceTimeSource(TimeSource::Unknown).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

TEST_F(MultiReaderTest, ReferenceDomainIdEqualityReferenceTimeSourceInequality15)
{
    constexpr const auto NUM_SIGNALS = 5;
    readSignals.reserve(NUM_SIGNALS);

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));
    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Tai).build()));
    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(
        0,
        113,
        createDomainSignal("1993",
                           nullptr,
                           nullptr,
                           ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Unknown).build()));

    addSignal(0,
              113,
              createDomainSignal("1993",
                                 nullptr,
                                 nullptr,
                                 ReferenceDomainInfoBuilder().setReferenceDomainId("A").setReferenceTimeSource(TimeSource::Gps).build()));

    ASSERT_THROW_MSG(
        MultiReader(signalsToList()), InvalidStateException, "Only one known Reference Time Source is allowed per Multi Reader.");
}

class MockSignal
{
public:
    MockSignal(const ContextPtr& context, const StringPtr& id, const StringPtr& epoch)
    {
        signal = daq::Signal(context, nullptr, id + "_valueSignal");
        domainSignal = daq::Signal(context, nullptr, id + "_domainSignal");

        auto valueDescriptor = daq::DataDescriptorBuilder()
                                   .setSampleType(daq::SampleType::Float64)
                                   .setUnit(Unit("V", -1, "volts", "voltage"))
                                   .setName(id + " values")
                                   .build();
        auto domainDescriptor = daq::DataDescriptorBuilder()
                                    .setSampleType(daq::SampleType::Int64)
                                    .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                    .setTickResolution(daq::Ratio(1, 1000))
                                    .setRule(daq::LinearDataRule(1, 0))
                                    .setOrigin(epoch)
                                    .setName(id + " time")
                                    .build();

        signal->setDescriptor(valueDescriptor);
        domainSignal->setDescriptor(domainDescriptor);
        signal->setDomainSignal(domainSignal);
    }
    SignalConfigPtr signal;
    SignalConfigPtr domainSignal;
};

TEST_F(MultiReaderTest, UndefinedReadWithMockSignals)
{
    StringPtr epoch = "2022-09-27T00:02:03+00:00";
    auto sig1 = MockSignal(context, "sig1", epoch);
    auto sig2 = MockSignal(context, "sig2", epoch);

    auto readerBuilder = MultiReaderBuilder();
    readerBuilder.addSignal(sig1.signal);
    readerBuilder.addSignal(sig2.signal);
    readerBuilder.setValueReadType(SampleType::Undefined);
    readerBuilder.setDomainReadType(SampleType::Int64);
    ASSERT_NO_THROW(readerBuilder.build());

    auto signalList = List<SignalPtr>(sig1.signal, sig2.signal);
    ASSERT_NO_THROW(MultiReader(signalList, SampleType::Undefined, SampleType::Int64));
}

TEST_F(MultiReaderTest, MultiReaderActive)
{
    constexpr auto NUM_SIGNALS = SizeT{3};
    constexpr auto NUM_SAMPLES = SizeT{10};
    double values[NUM_SIGNALS][NUM_SAMPLES] = {};
    double* valuesPerSignal[NUM_SIGNALS] = {values[0], values[1], values[2]};
    int64_t domainValues[NUM_SIGNALS][NUM_SAMPLES] = {};
    int64_t* domainValuesPerSignal[NUM_SIGNALS] = {domainValues[0], domainValues[1], domainValues[2]};
    auto count = SizeT{0};
    auto packetIndex = SizeT{0};

    readSignals.reserve(NUM_SIGNALS);

    auto signalReader = addSignal(0, NUM_SAMPLES, createDomainSignal());
    addSignal(0, NUM_SAMPLES, createDomainSignal());
    addSignal(0, NUM_SAMPLES, createDomainSignal());

    auto portList = portsList();
    auto multiReader = MultiReaderFromPort(portList);
    auto status = daq::MultiReaderStatusPtr();

    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList[i].connect(readSignals[i].signal);

    // check active status
    bool isActive = multiReader.getActive();
    ASSERT_TRUE(isActive);

    // send packets to active reader
    sendPackets(packetIndex++);  // 0

    // receive event packets
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Event);
    ASSERT_EQ(count, 0);

    // receive data packets
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, NUM_SAMPLES);

    multiReader.setActive(false);

    // send packets to inactive reader
    sendPackets(packetIndex++);  // 1
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, 0);

    // send packets to inactive reader
    sendPackets(packetIndex++);  // 2
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, 0);

    // send event packet
    auto signal = signalReader.signal;
    auto domainSignal = signal.getDomainSignal().asPtrOrNull<ISignalConfig>();

    ASSERT_TRUE(domainSignal.assigned());

    auto dataDescriptor = signal.getDescriptor();
    auto domainDescriptor = domainSignal.getDescriptor();
    auto newDomainDescriptor = DataDescriptorBuilderCopy(domainDescriptor).setSampleType(daq::SampleType::Int32).build();
    domainSignal.setDescriptor(newDomainDescriptor);

    // send packets to inactive reader
    sendPackets(packetIndex++);  // 3
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Event);
    ASSERT_EQ(count, 0);

    // set multireader active again
    multiReader.setActive(true);

    sendPackets(packetIndex++);  // 4
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, NUM_SAMPLES);
}

TEST_F(MultiReaderTest, MultiReaderActiveCopyInactive)
{
    using namespace std::chrono_literals;

    constexpr auto NUM_SIGNALS = SizeT{3};
    constexpr auto NUM_SAMPLES = SizeT{10};
    double values[NUM_SIGNALS][NUM_SAMPLES] = {};
    double* valuesPerSignal[NUM_SIGNALS] = {values[0], values[1], values[2]};
    int64_t domainValues[NUM_SIGNALS][NUM_SAMPLES] = {};
    int64_t* domainValuesPerSignal[NUM_SIGNALS] = {domainValues[0], domainValues[1], domainValues[2]};
    auto count = SizeT{0};

    readSignals.reserve(NUM_SIGNALS);

    auto signalReader = addSignal(0, NUM_SAMPLES, createDomainSignal());
    addSignal(0, NUM_SAMPLES, createDomainSignal());
    addSignal(0, NUM_SAMPLES, createDomainSignal());

    auto portList = portsList();
    auto multiReader = MultiReaderFromPort(portList);
    auto status = daq::MultiReaderStatusPtr();

    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList[i].connect(readSignals[i].signal);

    // send packets to active reader
    SizeT packetIndex = 0;
    sendPackets(packetIndex++);  // 0

    // receive event packets
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Event);
    ASSERT_EQ(count, 0);

    // set inactive, try read and copy reader
    multiReader.setActive(false);

    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, 0);

    auto multiReaderNew = MultiReaderFromExisting(multiReader);

    ASSERT_FALSE(multiReaderNew.getActive());

    // send packets to inactive copy of multireader
    sendPackets(packetIndex++);  // 1

    count = NUM_SAMPLES;
    status = multiReaderNew.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, 0);

    // set new multireader active and try to read samples
    multiReaderNew.setActive(true);

    sendPackets(packetIndex++);  // 1

    count = NUM_SAMPLES;
    status = multiReaderNew.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);

    ASSERT_EQ(status.getReadStatus(), daq::ReadStatus::Ok);
    ASSERT_EQ(count, NUM_SAMPLES);
}

TEST_F(MultiReaderTest, MultiReaderActiveFromPorts)
{
    using namespace std::chrono_literals;

    constexpr auto NUM_SIGNALS = SizeT{3};
    constexpr auto NUM_SAMPLES = SizeT{10};

    readSignals.reserve(NUM_SIGNALS);

    auto signalReader = addSignal(0, NUM_SAMPLES, createDomainSignal());
    addSignal(0, NUM_SAMPLES, createDomainSignal());
    addSignal(0, NUM_SAMPLES, createDomainSignal());

    auto portList = portsList();
    auto multiReader = MultiReaderFromPort(portList);
    auto status = daq::MultiReaderStatusPtr();

    multiReader.setActive(false);

    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList[i].connect(readSignals[i].signal);

    ASSERT_FALSE(multiReader.getActive());
    for (const auto& port : portList)
        ASSERT_FALSE(port.getActive());
}

TEST_F(MultiReaderTest, MultiReaderActiveGapPacket)
{
    using namespace std::chrono_literals;

    constexpr auto NUM_SIGNALS = SizeT{3};
    constexpr auto NUM_SAMPLES = SizeT{10};
    double values[NUM_SIGNALS][NUM_SAMPLES] = {};
    double* valuesPerSignal[NUM_SIGNALS] = {values[0], values[1], values[2]};
    int64_t domainValues[NUM_SIGNALS][NUM_SAMPLES] = {};
    int64_t* domainValuesPerSignal[NUM_SIGNALS] = {domainValues[0], domainValues[1], domainValues[2]};
    auto count = SizeT{0};
    auto packetIndex = SizeT{0};

    readSignals.reserve(NUM_SIGNALS);

    auto signalReader0 = addSignal(0, NUM_SAMPLES, createDomainSignal());
    auto signalReader1 = addSignal(0, NUM_SAMPLES, createDomainSignal());
    auto signalReader2 = addSignal(0, NUM_SAMPLES, createDomainSignal());

    auto portList = portsList(true);
    auto multiReader = MultiReaderFromPort(portList);
    auto status = daq::MultiReaderStatusPtr();

    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList[i].connect(readSignals[i].signal);

    // 0
    signalReader0.createAndSendPacket(packetIndex);
    signalReader1.createAndSendPacket(packetIndex);
    signalReader2.createAndSendPacket(packetIndex);

    // 1
    packetIndex += 1;
    // signalReader0.createAndSendPacket(packetIndex); // gap
    signalReader1.createAndSendPacket(packetIndex);
    signalReader2.createAndSendPacket(packetIndex);

    // 2
    packetIndex += 1;
    signalReader0.createAndSendPacket(packetIndex);
    signalReader1.createAndSendPacket(packetIndex);
    signalReader2.createAndSendPacket(packetIndex);

    // read first event packets
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(count, 0);

    // 10 samples until gap event
    ASSERT_EQ(multiReader.getAvailableCount(), 10);

    multiReader.setActive(false);

    ASSERT_EQ(multiReader.getAvailableCount(), 0);

    // read nothing, gap packet was dropped
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 0);

    // change descriptor
    auto signal = signalReader0.signal;
    auto domainSignal = signal.getDomainSignal().asPtrOrNull<ISignalConfig>();
    auto dataDescriptor = signal.getDescriptor();
    auto domainDescriptor = domainSignal.getDescriptor();
    auto newDomainDescriptor = DataDescriptorBuilderCopy(domainDescriptor).setSampleType(daq::SampleType::Int32).build();
    domainSignal.setDescriptor(newDomainDescriptor);

    // read descriptor changed event
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(count, 0);

    // set active again
    multiReader.setActive(true);

    // send data
    packetIndex += 1;
    signalReader0.createAndSendPacket(packetIndex);
    signalReader1.createAndSendPacket(packetIndex);
    signalReader2.createAndSendPacket(packetIndex);

    // read data
    count = NUM_SAMPLES;
    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, NUM_SAMPLES);
}

TEST_F(MultiReaderTest, MultiReaderActiveDataAvailableCallback)
{
    using namespace std::chrono_literals;

    constexpr auto NUM_SIGNALS = SizeT{3};
    constexpr auto NUM_SAMPLES = SizeT{10};
    double values[NUM_SIGNALS][NUM_SAMPLES] = {};
    double* valuesPerSignal[NUM_SIGNALS] = {values[0], values[1], values[2]};
    int64_t domainValues[NUM_SIGNALS][NUM_SAMPLES] = {};
    int64_t* domainValuesPerSignal[NUM_SIGNALS] = {domainValues[0], domainValues[1], domainValues[2]};

    readSignals.reserve(NUM_SIGNALS);

    auto signalReader0 = addSignal(0, NUM_SAMPLES, createDomainSignal());
    auto signalReader1 = addSignal(0, NUM_SAMPLES, createDomainSignal());
    auto signalReader2 = addSignal(0, NUM_SAMPLES, createDomainSignal());

    auto portList = portsList(true);
    auto multiReader = MultiReaderFromPort(portList);
    auto status = daq::MultiReaderStatusPtr();

    auto changeDomainSampleType = [](const ReadSignal& signalReader, SampleType newSampleType)
    {
        auto signal = signalReader.signal;
        auto domainSignal = signal.getDomainSignal().asPtrOrNull<ISignalConfig>();
        auto dataDescriptor = signal.getDescriptor();
        auto domainDescriptor = domainSignal.getDescriptor();
        auto newDomainDescriptor = DataDescriptorBuilderCopy(domainDescriptor).setSampleType(newSampleType).build();
        domainSignal.setDescriptor(newDomainDescriptor);
    };

    auto notified = false;
    auto state = 0;
    auto cv = std::condition_variable{};
    auto m = std::mutex{};

    multiReader.setOnDataAvailable(
        [this,
         &multiReader,
         &cv,
         &m,
         &notified,
         &state,
         &valuesPerSignal,
         &domainValuesPerSignal,
         NUM_SAMPLES]
        {
            auto lck = std::unique_lock{m};
            ASSERT_FALSE(notified);
            
            switch (state)
            {
                case 0:
                {
                    auto count = NUM_SAMPLES;
                    auto status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
                    ASSERT_EQ(count, 0);
                    auto events = status.getEventPackets();
                    for (auto i = SizeT{0}; i < events.getCount(); ++i)
                    {
                        auto sigId = fmt::format("/readsig{}", i);
                        ASSERT_TRUE(events.hasKey(sigId));
                        auto eventPacket = events.get(sigId);
                        ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
                        ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DATA_DESCRIPTOR));
                        ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DOMAIN_DATA_DESCRIPTOR));
                        auto domainDescriptor =
                            eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR).asPtrOrNull<IDataDescriptor>();
                        ASSERT_TRUE(domainDescriptor.assigned());
                        ASSERT_EQ(domainDescriptor.getSampleType(), SampleTypeFromType<ClockTick>::SampleType);
                    }

                    state = 1;
                    break;
                }
                case 1:
                {
                    auto count = NUM_SAMPLES;
                    auto status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
                    ASSERT_EQ(count, NUM_SAMPLES);

                    state = 2;
                    break;
                }
                case 2:
                {
                    ASSERT_EQ(multiReader.getAvailableCount(), 0);
                    auto count = NUM_SAMPLES;
                    auto status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
                    ASSERT_EQ(count, 0);
                    ASSERT_EQ(multiReader.getAvailableCount(), 10);

                    auto events = status.getEventPackets();
                    ASSERT_GE(events.getCount(), 1);

                    auto sigId = fmt::format("/readsig{}", 0);
                    ASSERT_TRUE(events.hasKey(sigId));

                    auto eventPacket = events.get(sigId);
                    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
                    ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DATA_DESCRIPTOR));
                    ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DOMAIN_DATA_DESCRIPTOR));

                    auto domainDescriptor =
                        eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR).asPtrOrNull<IDataDescriptor>();
                    ASSERT_TRUE(domainDescriptor.assigned());
                    ASSERT_EQ(domainDescriptor.getSampleType(), SampleType::UInt64);

                    multiReader.setActive(false);

                    state = 3;
                    break;
                }
                case 3:
                {
                    ASSERT_EQ(multiReader.getAvailableCount(), 0);
                    auto count = NUM_SAMPLES;
                    auto status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
                    ASSERT_EQ(count, 0);
                    ASSERT_EQ(multiReader.getAvailableCount(), 0);

                    auto events = status.getEventPackets();
                    ASSERT_GE(events.getCount(), 1);

                    auto sigId = fmt::format("/readsig{}", 0);
                    ASSERT_TRUE(events.hasKey(sigId));

                    auto eventPacket = events.get(sigId);
                    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
                    ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DATA_DESCRIPTOR));
                    ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DOMAIN_DATA_DESCRIPTOR));

                    auto domainDescriptor =
                        eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR).asPtrOrNull<IDataDescriptor>();
                    ASSERT_TRUE(domainDescriptor.assigned());
                    ASSERT_EQ(domainDescriptor.getSampleType(), SampleType::Float64);

                    state = 4;
                    break;
                }
                case 4:
                {
                    ASSERT_EQ(multiReader.getAvailableCount(), 0);
                    auto count = NUM_SAMPLES;
                    auto status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
                    ASSERT_EQ(count, 0);
                    ASSERT_EQ(multiReader.getAvailableCount(), 0);

                    auto events = status.getEventPackets();
                    ASSERT_GE(events.getCount(), 1);

                    auto sigId = fmt::format("/readsig{}", 0);
                    ASSERT_TRUE(events.hasKey(sigId));

                    auto eventPacket = events.get(sigId);
                    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
                    ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DATA_DESCRIPTOR));
                    ASSERT_TRUE(eventPacket.getParameters().hasKey(event_packet_param::DOMAIN_DATA_DESCRIPTOR));

                    auto domainDescriptor =
                        eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR).asPtrOrNull<IDataDescriptor>();
                    ASSERT_TRUE(domainDescriptor.assigned());
                    ASSERT_EQ(domainDescriptor.getSampleType(), SampleType::Int64);

                    multiReader.setActive(true);

                    state = 5;
                    break;
                }
                case 5:
                {
                    ASSERT_EQ(multiReader.getAvailableCount(), 0);
                    auto count = NUM_SAMPLES;
                    auto status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
                    ASSERT_EQ(count, 0);

                    count = NUM_SAMPLES;
                    status = multiReader.readWithDomain(valuesPerSignal, domainValuesPerSignal, &count);
                    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
                    ASSERT_EQ(count, NUM_SAMPLES);

                    state = 6;
                    break;
                }
                default:
                {
                    GTEST_FAIL();
                }
            }
            notified = true;
            cv.notify_all();
        });

    for (size_t i = 0; i < NUM_SIGNALS; i++)
        portList[i].connect(readSignals[i].signal);

    while (state != 6)
    {
        auto lck = std::unique_lock{m};
        auto cv_status = cv.wait_for(lck, 10s, [&notified] { return notified; });
        ASSERT_TRUE(cv_status);
        notified = false;

        switch (state)
        {
            case 1:
                signalReader0.createAndSendPacket(0);
                signalReader1.createAndSendPacket(0);
                signalReader2.createAndSendPacket(0);
                break;
            case 2:
                changeDomainSampleType(signalReader0, SampleType::UInt64);
                signalReader0.createAndSendPacket(1);
                signalReader1.createAndSendPacket(1);
                signalReader2.createAndSendPacket(1);
                break;
            case 3:
                changeDomainSampleType(signalReader0, SampleType::Float64);
                signalReader0.createAndSendPacket(2);
                signalReader1.createAndSendPacket(2);
                signalReader2.createAndSendPacket(2);
                break;
            case 4:
                changeDomainSampleType(signalReader0, SampleType::Int64);
                break;
            case 5:
                signalReader0.createAndSendPacket(3);
                signalReader1.createAndSendPacket(3);
                signalReader2.createAndSendPacket(3);
                break;
            case 6:
                // finish
                break;
            default:
                GTEST_FAIL();
                break;
        }
    }

    context.getScheduler().waitAll();

    ASSERT_EQ(state, 6);
}

TEST_F(MultiReaderTest, ExpectSR)
{
    const auto ctx = NullContext();

    auto valueDesc = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();
    auto timeDesc = DataDescriptorBuilder()
                        .setSampleType(SampleType::Int64)
                        .setRule(LinearDataRule(0, 10))
                        .setTickResolution(Ratio(1, 1000))
                        .setUnit(Unit("s", -1, "second", "time"))
                        .build();

    const auto valueSignal = SignalWithDescriptor(ctx, valueDesc, nullptr, "value");
    const auto timeSignal = SignalWithDescriptor(ctx, timeDesc, nullptr, "time");
    valueSignal.setDomainSignal(timeSignal);

    const auto reader = MultiReaderBuilder().addSignal(valueSignal).setDomainReadType(SampleType::Int64).setValueReadType(SampleType::Int32).setRequiredCommonSampleRate(10).build();

    size_t count = 0;
    auto status = reader.read(nullptr, &count, 0);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    count = 0;
    status = reader.read(nullptr, &count, 0);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);
}

TEST_F(MultiReaderTest, TestReaderWithConnectedPortConnectionEmpty)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const SizeT SAMPLES = 4u;

    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 30, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 40, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();
    portList[0].connect(sig0.signal);
    portList[1].connect(sig1.signal);
    portList[2].connect(sig2.signal);

    for (const auto & port : portList)
    {
        auto connection = port.getConnection();
        ASSERT_TRUE(connection.assigned());
        
        SizeT packetInConnection = 0;
        while (true)
        {
            if (connection.dequeue().assigned())
                packetInConnection++;
            else
                break;
        }

        // 1 event packet
        ASSERT_EQ(packetInConnection, 1u);
    }
    auto reader = MultiReaderFromPort(portList);

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
        ASSERT_EQ(status.getEventPackets().getCount(), 3u);
    }

    for (const auto & port : portList)
    {
        auto signal = port.getSignal().asPtr<ISignalConfig>(true);
        ASSERT_TRUE(signal.assigned());

        auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), SAMPLES, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, signal.getDescriptor(), SAMPLES);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        for (SizeT i = 0; i < SAMPLES; i++)
            dataPtr[i] = 111.1 * (i + 1);

        signal.sendPacket(dataPacket);
    }

    {
        std::array<double[SAMPLES], NUM_SIGNALS> values{};
        void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};

        SizeT count{SAMPLES};
        auto status = reader.read(&valuesPerSignal, &count, 500);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

        ASSERT_EQ(count, SAMPLES);
        for (SizeT i = 0; i < SAMPLES; i++)
        {
            ASSERT_EQ(values[0][i], 111.1 * (i + 1));
            ASSERT_EQ(values[1][i], 111.1 * (i + 1));
            ASSERT_EQ(values[2][i], 111.1 * (i + 1));
        }
    }
}

TEST_F(MultiReaderTest, TestReaderWithConnectedPortConnectionNotEmpty)
{
    constexpr const auto NUM_SIGNALS = 3;
    constexpr const SizeT SAMPLES = 4u;

    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 20, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 30, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 40, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = portsList();
    portList[0].connect(sig0.signal);
    portList[1].connect(sig1.signal);
    portList[2].connect(sig2.signal);

    for (const auto & port : portList)
    {
        auto connection = port.getConnection();
        ASSERT_TRUE(connection.assigned());
        
        SizeT packetInConnection = 0;
        while (true)
        {
            if (connection.dequeue().assigned())
                packetInConnection++;
            else
                break;
        }

        // 1 event packet
        ASSERT_EQ(packetInConnection, 1u);
    }

    for (const auto & port : portList)
    {
        auto signal = port.getSignal().asPtr<ISignalConfig>(true);
        ASSERT_TRUE(signal.assigned());

        auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), SAMPLES, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, signal.getDescriptor(), SAMPLES);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        for (SizeT i = 0; i < SAMPLES; i++)
            dataPtr[i] = 111.1 * (i + 1);

        signal.sendPacket(dataPacket);
    }

    auto reader = MultiReaderFromPort(portList);

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
        ASSERT_EQ(status.getEventPackets().getCount(), 3u);
    }

    {
        std::array<double[SAMPLES], NUM_SIGNALS> values{};
        void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};

        SizeT count{SAMPLES};
        auto status = reader.read(&valuesPerSignal, &count, 500);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

        ASSERT_EQ(count, SAMPLES);
        for (SizeT i = 0; i < SAMPLES; i++)
        {
            ASSERT_EQ(values[0][i], 111.1 * (i + 1));
            ASSERT_EQ(values[1][i], 111.1 * (i + 1));
            ASSERT_EQ(values[2][i], 111.1 * (i + 1));
        }
    }
}

class MinReadCountTest : public MultiReaderTest, public testing::WithParamInterface<SizeT>
{
};

TEST_P(MinReadCountTest, MinReadCount)
{
    auto timeoutMs = GetParam();

    constexpr auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    auto& sig0 = addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig2 = addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto multi = MultiReaderBuilder().addSignal(sig0.signal).addSignal(sig1.signal).addSignal(sig2.signal).setMinReadCount(20).build();

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);

    constexpr const SizeT SAMPLES = 20u;
    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    int64_t domain[SAMPLES];

    SizeT count{10};
    MultiReaderStatusPtr status;

    ASSERT_EQ(multi.getAvailableCount(), 0);

    ASSERT_THROW(multi.read(valuesPerSignal, &count, timeoutMs), InvalidParameterException);
    count = 10;
    ASSERT_THROW(multi.skipSamples(&count), InvalidParameterException);
    count = 10;
    ASSERT_THROW(multi.readWithDomain(valuesPerSignal, domain, &count, timeoutMs), InvalidParameterException);

    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);
    sig2.createAndSendPacket(1);

    ASSERT_EQ(multi.getAvailableCount(), 0);
    count = 0;
    status = multi.read(nullptr, &count, timeoutMs);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    ASSERT_EQ(multi.getAvailableCount(), 20);

    count = 20;
    multi.read(valuesPerSignal, &count, timeoutMs);

    ASSERT_EQ(count, 20);

    sig0.createAndSendPacket(2);
    sig1.createAndSendPacket(2);
    sig2.createAndSendPacket(2);

    ASSERT_EQ(multi.getAvailableCount(), 0);

    sig0.setValueDescriptor(setupDescriptor(SampleType::Int32));

    ASSERT_EQ(multi.getAvailableCount(), 0);

    count = 0;
    status = multi.read(nullptr, &count, timeoutMs);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    count = 0;
    status = multi.read(nullptr, &count, timeoutMs);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(count, 0);
}

INSTANTIATE_TEST_SUITE_P(MinReadCountSuite, MinReadCountTest, testing::Values(0, 1000));

TEST_F(MultiReaderTest, TestTickOffsetExceeded)
{
    using namespace std::chrono_literals;
    constexpr auto kPacketSize = SizeT{10};
    constexpr auto kSignalCount = SizeT{10};
    const auto resolution = Ratio(1, 10);

    const auto okTolerance = Ratio(10, 100);
    const auto failTolerance = Ratio(8, 100);
    const auto boundTolerance = Ratio(9, 100);

    auto epoch = std::chrono::system_clock::now();
    auto dataBuffers = std::vector<void*>(kSignalCount, nullptr);
    auto domainBuffers = std::vector<void*>(kSignalCount, nullptr);

    for (auto i = 0; i < kSignalCount; ++i)
    {
        // auto epochString = reader::timePointString(epoch);
        auto epochString = date::format("%FT%T%z", epoch);
        auto domainSignal = createDomainSignal(epochString, resolution, LinearDataRule(1, 0));
        auto domainSampleSize = domainSignal.getDescriptor().getSampleSize();
        auto dataSignal = addSignal(0, kPacketSize, domainSignal);
        auto dataSampleSize = dataSignal.signal.getDescriptor().getSampleSize();
        domainBuffers[i] = std::calloc(kPacketSize, domainSampleSize);
        dataBuffers[i] = std::calloc(kPacketSize, dataSampleSize);
        epoch += 10ms;
    }

    auto multiReaderBuilder = MultiReaderBuilder();
    auto ports = portsList();
    auto signals = signalsToList();
    ASSERT_EQ(ports.getCount(), signals.getCount());
    for (auto i = 0; i < ports.getCount(); ++i)
    {
        ports[i].connect(signals[i]);
        multiReaderBuilder.addInputPort(ports[i]);
    }
    multiReaderBuilder.setTickOffsetTolerance(failTolerance);

    auto multiReader = multiReaderBuilder.build();

    for (auto& signal: readSignals)
        signal.createAndSendPacket(0);

    auto count = SizeT{0};
    auto status = multiReader.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    count = 10;
    status = multiReader.readWithDomain(dataBuffers.data(), domainBuffers.data(), &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(multiReader.getActive(), false);
}
