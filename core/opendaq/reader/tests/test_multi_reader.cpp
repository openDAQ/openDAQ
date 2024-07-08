#include <opendaq/reader_exceptions.h>
#include <opendaq/reader_factory.h>
#include <opendaq/time_reader.h>
#include "reader_common.h"
#include <opendaq/event_packet_params.h>
#include <opendaq/input_port_factory.h>

#include <gmock/gmock-matchers.h>

#include <thread>
#include <utility>
#include <future>

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

    [[nodiscard]]
    SignalConfigPtr getDomainSignal() const
    {
        return signal.getDomainSignal();
    }

    [[nodiscard]]
    auto getDomainDescriptor() const
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

        auto dataDescriptor = domainDataDescriptor.assigned()
                                  ? domainDataDescriptor
                                  : getDomainDescriptor();

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
            std::cout
                << "<" << packetIndex << "> "
                <<"(off: " << offset << " pSize: " << packetSize << " pOffset: " << packetOffset << ")"
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

    daq::LoggerPtr getLogger() override
    {
        LoggerSinkPtr sink;

#if defined(_WIN32)
        sink = WinDebugLoggerSink();
#else
        sink = StdOutLoggerSink();
#endif

#if defined(NDEBUG)
        sink.setLevel(LogLevel::Info);
#else
        sink.setLevel(LogLevel::Trace);
#endif

        return LoggerWithSinks(List<ILoggerSink>(sink), LogLevel::Trace);
    }

    ReadSignal& addSignal(Int packetOffset, Int packetSize, const SignalPtr& domain, SampleType valueType = SampleType::Float64)
    {
        auto newSignal = Signal(context, nullptr, fmt::format("sig{}", counter++));
        newSignal.setDescriptor(setupDescriptor(valueType));
        newSignal.setDomainSignal(domain);

        return readSignals.emplace_back(newSignal, packetOffset, packetSize);
    }

    [[nodiscard]]
    SignalConfigPtr createDomainSignal(std::string epoch = "",
                                       const daq::RatioPtr& resolution = nullptr,
                                       const daq::DataRulePtr& rule = nullptr) const
    {
        auto domain = Signal(context, nullptr, "time");
        domain.setDescriptor(createDomainDescriptor(std::move(epoch), resolution, rule));

        return domain;
    }

    void sendPackets(Int index) const
    {
        for (const auto& signal : readSignals)
        {
            signal.createAndSendPacket(index);
        }
    }

    [[nodiscard]]
    ListPtr<ISignal> signalsToList() const
    {
        ListPtr<SignalConfigPtr> signals = List<ISignalConfig>();
        for (const auto& read : readSignals)
        {
            signals.pushBack(read.signal);
        }
        return signals;
    }

    [[nodiscard]]
    ListPtr<IInputPortConfig> signalsToPortsList(bool enableGapDetection = false) const
    {
        ListPtr<IInputPortConfig> ports = List<IInputPortConfig>();
        for (const auto& read : readSignals)
        {
            auto port = InputPort(read.signal.getContext(), nullptr, "readsig", enableGapDetection);
            port.connect(read.signal);
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

    std::thread thread([sig0, sig1, sig2]
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

    std::thread thread([sig0, sig1, sig2] {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(200ms);

        sig0.createAndSendPacket(3);
        sig1.createAndSendPacket(3);
        sig2.createAndSendPacket(3);
    });

    auto start = std::chrono::system_clock::now();

    SizeT count{SAMPLES};
    multi.readWithDomain(valuesPerSignal, domainPerSignal, &count, 300);

    auto end = std::chrono::system_clock::now();

    if (thread.joinable())
        thread.join();

    ASSERT_THAT(end - start, AllOf(Gt(299ms), Le(350ms)));
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
    auto& sig1 = addSignal(134, 732,              createDomainSignal(" "));
    auto& sig2 = addSignal(111, 843,              createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

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
    auto& sig1 = addSignal(0, 732,              createDomainSignal(" ", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(0, 843,              createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

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
    auto& sig1 = addSignal(130, 732,  createDomainSignal(" ", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(111, 843,  createDomainSignal(" "));

    auto multi = MultiReader(signalsToList());

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
    auto& sig1 = addSignal(131, 732,                createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0)));
    auto& sig2 = addSignal(111, 843,                createDomainSignal("2022-09-27T00:02:04.123+00:00"));

    auto multi = MultiReader(signalsToList());

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

    SizeT count{1};
    MultiReaderStatusPtr status = multi.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_TRUE(status.getEventPackets().assigned());
    ASSERT_EQ(status.getEventPackets().getCount(), 1u);
    ASSERT_TRUE(status.getEventPackets().hasKey(sig1.signal));
    ASSERT_NE(status.getEventPackets().get(sig1.signal), nullptr);

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

    // Read over the signal-descriptor change. it will stops on event. so it will read 632 as getAvailableCount return synced samples until event
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

    auto multi = MultiReaderFromPort(signalsToPortsList());

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
    
    auto multi = MultiReaderFromPort(signalsToPortsList());

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    portList[0].connect(sig0.signal);
    portList[1].connect(sig1.signal);
    portList[2].connect(sig2.signal);

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

    auto portList = signalsToPortsList();
    auto componentList = List<IComponent>(portList[0], portList[1], sig2.signal);

    ASSERT_THROW(MultiReaderFromPort(componentList), InvalidParameterException);
}

TEST_F(MultiReaderTest, MultipleMultiReaderToInputPort)
{
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(1);

    addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = signalsToPortsList();

    auto reader1 = MultiReaderFromPort(portList);
    ASSERT_THROW(MultiReaderFromPort(portList), AlreadyExistsException);
}

TEST_F(MultiReaderTest, MultiReaderReuseInputPort)
{
    // prevent vector from re-allocating, so we have "stable" pointers
    readSignals.reserve(1);

    addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto portList = signalsToPortsList();

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

    reader.setOnDataAvailable([&, promise = &promise] () mutable {
        if (reader.getAvailableCount() < count)
            return;

        reader.readWithDomain(valuesPerSignal, domainPerSignal, &count);
        reader.setOnDataAvailable(nullptr); // trigger callback only once
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

    auto reader = MultiReaderFromPort(signalsToPortsList());

    reader.setOnDataAvailable([&, promise = &promise] () mutable {
        if (reader.getAvailableCount() < count)
            return;

        reader.readWithDomain(valuesPerSignal, domainPerSignal, &count);
        reader.setOnDataAvailable(nullptr); // trigger callback only once
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

    std::array<std::int32_t, NUM_SIGNALS> dividers = { 1, 2, 5 };

    auto& sig0 = addSignal(0, 523, createDomainSignal("2022-09-27T00:02:03+00:00", nullptr, LinearDataRule(dividers[0], 0)));     // 1000 Hz
    auto& sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", nullptr, LinearDataRule(dividers[1], 0)));     // 500 Hz
    auto& sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.125+00:00", nullptr, LinearDataRule(dividers[2], 0))); // 200 Hz

    auto multi = MultiReader(signalsToList());
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

    auto multi = MultiReaderEx(signalsToList(), SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All, reqiredRate, false);
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
    SignalPtr sig1 = addSignal(0, 732, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 1000 * 10ll), LinearDataRule(10, 0))).signal;
    SignalPtr sig2 = addSignal(0, 843, createDomainSignal("2022-09-27T00:02:04.123+00:00")).signal;

    auto portList = signalsToPortsList();

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
    ASSERT_EQ(builder.getSourceComponents()[0].asPtr<IInputPort>().getSignal(), sig0);
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

    auto portList = signalsToPortsList();

    SignalPtr signal1 = sig1.signal;
    SignalPtr signal2 = sig2.signal;

    MultiReaderBuilderPtr builder = MultiReaderBuilder().addInputPort(portList[0]).addSignal(signal1).addSignal(signal2);
    ASSERT_THROW(MultiReaderFromBuilder(builder), InvalidParameterException);
}

TEST_F(MultiReaderTest, MultiReaderExcetionOnConstructor)
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

    constexpr size_t numberOfSamplesToRead = 8;
    double dataFirstSignal[numberOfSamplesToRead];
    double dataSecondSignal[numberOfSamplesToRead];
    double* data[2] { dataFirstSignal, dataSecondSignal };

    size_t count = numberOfSamplesToRead;
    auto a1 = std::async (std::launch::async, [&] {
        multiReader.read(data, &count, 10000);
    });

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

    auto multi = MultiReader(signalsToPortsList(true));

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

    auto available = multi.getAvailableCount();
    ASSERT_EQ(available, 0u);

    SizeT count{0};
    MultiReaderStatusPtr status = multi.read(nullptr, &count);
    ASSERT_TRUE(status.assigned());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_TRUE(status.getEventPackets().assigned());
    ASSERT_EQ(status.getEventPackets().getCount(), 1u);
    ASSERT_TRUE(status.getEventPackets().hasKey(sig0.signal));
    
    auto event = status.getEventPackets().get(sig0.signal);
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
