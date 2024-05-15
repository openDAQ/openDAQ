#include <opendaq/opendaq.h>

using namespace daq;

/*
 * Corresponding document: Antora/modules/howto_guides/pages/howto_read_aligned_signals.adoc
 */

struct ReadSignal
{
    explicit ReadSignal(const SignalConfigPtr& signal, std::int64_t packetSize);
    void sendPacket();

    int packetIndex{0};
    std::int64_t packetSize;

    SignalConfigPtr signal;
    DataDescriptorPtr valueDescriptor;
};

template <typename T, typename U>
void printData(std::int64_t samples, T& times, U& values);

SignalConfigPtr createDomainSignal(const ContextPtr& context, std::string epoch);
ReadSignal demoSignal(const ContextPtr& context, std::int64_t packetSize, const std::string& domainOrigin);

/*
 * Aligns 3 Signals to the same Domain position and starts reading from there
 */
void exampleSimple()
{
    constexpr const auto NUM_SIGNALS = 3;

    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);

    ReadSignal sig0 = demoSignal(context, 523, "2022-09-27T00:02:03+00:00");
    ReadSignal sig1 = demoSignal(context, 732, "2022-09-27T00:02:04+00:00");
    ReadSignal sig2 = demoSignal(context, 843, "2022-09-27T00:02:04.123+00:00");

    ListPtr<ISignal> signals{sig0.signal, sig1.signal, sig2.signal};
    auto reader = MultiReader(signals);

    sig0.sendPacket();
    sig1.sendPacket();
    sig2.sendPacket();

    [[maybe_unused]] auto available = reader.getAvailableCount();  // 0
    assert(available == 0);

    sig0.sendPacket();
    sig1.sendPacket();
    sig2.sendPacket();

    sig0.sendPacket();
    sig1.sendPacket();
    sig2.sendPacket();

    // Samples per Signal
    // 523 * 3 = 1569 (1.569s) need 1123 to sync
    // 732 * 3 = 2196 (2.196s) need  123 to sync
    // 843 * 3 = 2529 (2.529s) need    0 to sync

    available = reader.getAvailableCount();  // 446
    assert(available == 446);

    constexpr const SizeT SAMPLES = 523;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<ClockTick[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count = SAMPLES;
    reader.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    // count = 446
    assert(count == 446);

    available = reader.getAvailableCount();  // 0
    assert(available == 0);

    /* Should print:
     *
     *   Signal 0
     *    |d: 1123 |v: 1123.0
     *    |d: 1124 |v: 1124.0
     *    |d: 1125 |v: 1125.0
     *    |d: 1126 |v: 1126.0
     *    |d: 1127 |v: 1127.0
     *   --------
     *   Signal 1
     *    |d: 123 |v: 123.0
     *    |d: 124 |v: 124.0
     *    |d: 125 |v: 125.0
     *    |d: 126 |v: 126.0
     *    |d: 127 |v: 127.0
     *   --------
     *   Signal 2
     *    |d: 0 |v: 0.0
     *    |d: 1 |v: 1.0
     *    |d: 2 |v: 2.0
     *    |d: 3 |v: 3.0
     *    |d: 4 |v: 4.0
     */

    printData(5, domain, values);
}

/*
 * The same as example 1 but read Domain in `std::chrono::system_clock::time_point` values
 */
void exampleWithTimeStamps()
{
    constexpr const auto NUM_SIGNALS = 3;

    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);

    ReadSignal sig0 = demoSignal(context, 523, "2022-09-27T00:02:03+00:00");
    ReadSignal sig1 = demoSignal(context, 732, "2022-09-27T00:02:04+00:00");
    ReadSignal sig2 = demoSignal(context, 843, "2022-09-27T00:02:04.123+00:00");

    ListPtr<ISignal> signals{sig0.signal, sig1.signal, sig2.signal};

    auto reader = MultiReader(signals);
    TimeReader timeReader(reader);

    sig0.sendPacket();
    sig1.sendPacket();
    sig2.sendPacket();

    [[maybe_unused]] auto available = reader.getAvailableCount();  // 0
    assert(available == 0);

    sig0.sendPacket();
    sig1.sendPacket();
    sig2.sendPacket();

    sig0.sendPacket();
    sig1.sendPacket();
    sig2.sendPacket();

    // Samples per Signal
    // 523 * 3 = 1569 (1.569s) need 1123 to sync
    // 732 * 3 = 2196 (2.196s) need  123 to sync
    // 843 * 3 = 2529 (2.529s) need    0 to sync

    available = reader.getAvailableCount();  // 446
    assert(available == 446);

    constexpr const SizeT SAMPLES = 523;

    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    std::array<std::chrono::system_clock::time_point[SAMPLES], NUM_SIGNALS> domain{};

    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};
    void* domainPerSignal[NUM_SIGNALS]{domain[0], domain[1], domain[2]};

    SizeT count = SAMPLES;
    reader.readWithDomain(valuesPerSignal, domainPerSignal, &count);
    // count = 446
    assert(count == 446);

    available = reader.getAvailableCount();  // 0
    assert(available == 0);

    /* Should print:
     *
     *  Signal 0
     *   |d: 2022-09-27 00:02:04.1230000 |v: 1123.0
     *   |d: 2022-09-27 00:02:04.1240000 |v: 1124.0
     *   |d: 2022-09-27 00:02:04.1250000 |v: 1125.0
     *   |d: 2022-09-27 00:02:04.1260000 |v: 1126.0
     *   |d: 2022-09-27 00:02:04.1270000 |v: 1127.0
     *  --------
     *  Signal 1
     *   |d: 2022-09-27 00:02:04.1230000 |v: 123.0
     *   |d: 2022-09-27 00:02:04.1240000 |v: 124.0
     *   |d: 2022-09-27 00:02:04.1250000 |v: 125.0
     *   |d: 2022-09-27 00:02:04.1260000 |v: 126.0
     *   |d: 2022-09-27 00:02:04.1270000 |v: 127.0
     *  --------
     *  Signal 2
     *   |d: 2022-09-27 00:02:04.1230000 |v: 0.0
     *   |d: 2022-09-27 00:02:04.1240000 |v: 1.0
     *   |d: 2022-09-27 00:02:04.1250000 |v: 2.0
     *   |d: 2022-09-27 00:02:04.1260000 |v: 3.0
     *   |d: 2022-09-27 00:02:04.1270000 |v: 4.0
     */

    printData(5, domain, values);
}

void drawBoxMessage(const std::string& message);

int main(int /*argc*/, const char* /*argv*/[])
{
    drawBoxMessage("Example 1");
    exampleSimple();

    drawBoxMessage("Example 2");
    exampleWithTimeStamps();
    return 0;
}

/*
 * Utility functions
 */

SignalConfigPtr createDomainSignal(const ContextPtr& context, std::string epoch)
{
    DataDescriptorPtr dataDescriptor = DataDescriptorBuilder()
                                           .setSampleType(SampleTypeFromType<ClockTick>::SampleType)
                                           .setOrigin(epoch)
                                           .setTickResolution(Ratio(1, 1000))
                                           .setRule(LinearDataRule(1, 0))
                                           .setUnit(Unit("s", -1, "seconds", "time"))
                                           .build();

    auto domain = Signal(context, nullptr, "time");
    domain.setDescriptor(dataDescriptor);

    return domain;
}

ReadSignal demoSignal(const ContextPtr& context, std::int64_t packetSize, const std::string& domainOrigin)
{
    static int counter = 0;

    auto newSignal = Signal(context, nullptr, fmt::format("sig{}", counter++));
    newSignal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());
    newSignal.setDomainSignal(createDomainSignal(context, domainOrigin));

    return ReadSignal(newSignal, packetSize);
}

ReadSignal::ReadSignal(const SignalConfigPtr& signal, std::int64_t packetSize)
    : packetSize(packetSize)
    , signal(signal)
    , valueDescriptor(signal.getDescriptor())
{
}

void ReadSignal::sendPacket()
{
    auto domainSignal = signal.getDomainSignal();
    auto domainDescriptor = domainSignal.getDescriptor();

    Int delta = domainDescriptor.getRule().getParameters()["delta"];

    auto offset = (packetSize * delta) * packetIndex;
    auto domainPacket = DataPacket(domainDescriptor, packetSize, offset);
    auto packet = DataPacketWithDomain(domainPacket, valueDescriptor, packetSize);

    // Zero-out data
    memset(packet.getRawData(), 0, packet.getRawDataSize());

    auto* data = static_cast<double*>(packet.getRawData());
    for (auto i = 0; i < packetSize; ++i)
    {
        data[i] = offset + i;
    }

    signal.sendPacket(packet);
    packetIndex++;
}

template <typename T, typename U>
void printData(std::int64_t samples, T& times, U& values)
{
    using namespace std::chrono;
    using namespace reader;

    int numSignals = std::size(times);
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

void drawBoxMessage(const std::string& message)
{
    fmt::print("┌{0:─^{2}}┐\n"
               "│{1: ^{2}}│\n"
               "└{0:─^{2}}┘\n",
               "",
               message,
               20);
}
