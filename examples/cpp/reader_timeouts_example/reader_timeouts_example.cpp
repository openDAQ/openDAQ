#include <opendaq/context_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>

#include <cassert>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

SignalConfigPtr setupExampleSignal();
DataDescriptorPtr setupDescriptor(SampleType type);
DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples);

/*
 * Corresponding document: Antora/modules/howto_guides/pages/howto_read_with_timeouts.adoc
 */

/*
 * Example 1: Reading with time-outs
 */
void example1(const SignalConfigPtr& signal)
{
    auto reader = StreamReaderBuilder()
                      .setSignal(signal)
                      .setValueReadType(SampleType::Float64)
                      .setSkipEvents(true)
                      .build();

    // Signal produces 2 samples
    auto packet1 = createPacketForSignal(signal, 2);
    signal.sendPacket(packet1);

    [[maybe_unused]] auto available = reader.getAvailableCount();
    assert(available == 2u);

    // Normal call will only return existing 2 samples immediately
    SizeT count{5};
    double values[5]{};
    reader.read(values, &count);  // count = 2

    assert(count == 2u);

    // Signal produces 2 samples, then in 100 ms after the read call another 2
    auto packet2 = createPacketForSignal(signal, 2);
    signal.sendPacket(packet2);

    std::thread t(
        [&signal]
        {
            std::this_thread::sleep_for(100ms);

            auto packet3 = createPacketForSignal(signal, 2);
            signal.sendPacket(packet3);
        });

    count = 5;
    double newValues[5]{};
    reader.read(newValues, &count, 200);  // count = 4

    if (t.joinable())
        t.join();

    assert(count == 4u);
}

/*
 * Example 2: Reading with the time-out option "Any"
 */
void example2(const SignalConfigPtr& signal)
{
    auto reader = StreamReaderBuilder()
                      .setSignal(signal)
                      .setValueReadType(SampleType::Float64)
                      .setReadTimeoutType(ReadTimeoutType::Any)
                      .setSkipEvents(true)
                      .build();

    // Signal produces 2 Packets with 3 samples
    // [Packet 1]: { 1 }
    // [Packet 2]: { 2, 3 }

    {
        auto packet1 = createPacketForSignal(signal, 1);
        auto data1 = static_cast<double*>(packet1.getData());
        data1[0] = 1;

        signal.sendPacket(packet1);

        auto packet2 = createPacketForSignal(signal, 2);
        auto data2 = static_cast<double*>(packet2.getData());
        data2[0] = 2;
        data2[1] = 3;

        signal.sendPacket(packet2);
    }

    [[maybe_unused]] auto available = reader.getAvailableCount();  // available = 3

    // Returns immediately with the currently available samples
    SizeT count{5};
    double values[5]{};
    reader.read(values, &count, 200);  // count = 3, values = { 1, 2, 3 }

    assert(count == 3u);
    assert(values[0] == 1);
    assert(values[1] == 2);
    assert(values[2] == 3);

    // There are no samples left in the Reader
    available = reader.getAvailableCount();  // available = 0
    assert(available == 0u);

    std::thread t(
        [&signal]
        {
            // 50 ms after the read call the Signal produces a Packet with 2 samples { 4, 5 }

            std::this_thread::sleep_for(50ms);

            auto packet3 = createPacketForSignal(signal, 2);
            auto data3 = static_cast<double*>(packet3.getData());
            data3[0] = 4;
            data3[1] = 5;

            signal.sendPacket(packet3);

            // then, after another 20 ms, produces the next 3 samples { 6, 7, 8 }

            std::this_thread::sleep_for(20ms);

            auto packet4 = createPacketForSignal(signal, 3);
            auto data4 = static_cast<double*>(packet4.getData());
            data4[0] = 6;
            data4[1] = 7;
            data4[2] = 8;
            signal.sendPacket(packet3);
        });

    count = {5};
    double newValues[5]{};
    reader.read(newValues, &count, 200);  // count = 2, newValues = { 4, 5 }

    if (t.joinable())
        t.join();

    assert(count == 2u);
    assert(newValues[0] == 4);
    assert(newValues[1] == 5);
}
/*
 * ENTRY POINT
 */
int main(int /*argc*/, const char* /*argv*/[])
{
    SignalConfigPtr signal = setupExampleSignal();

    example1(signal);
    example2(signal);

    return 0;
}

/*
 * Set up the Signal with Float64 data
 */
SignalConfigPtr setupExampleSignal()
{
    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);

    auto signal = Signal(context, nullptr, "example signal");
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    return signal;
}

DataDescriptorPtr setupDescriptor(SampleType type)
{
    // Set-up the Data Descriptor with the provided Sample Type
    return DataDescriptorBuilder().setSampleType(type).build();
}

DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples)
{
    return daq::DataPacket(signal.getDescriptor(), numSamples);
}
