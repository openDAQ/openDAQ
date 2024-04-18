#include <opendaq/context_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>

#include <iostream>
#include <cassert>

using namespace daq;

SignalConfigPtr setupExampleSignal();
DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples, Int offset = 0);
DataDescriptorPtr setupDescriptor(daq::SampleType type, daq::DataRulePtr rule = nullptr);

/*
 * Corresponding document: Antora/modules/howto_guides/pages/howto_read_last_n_samples.adoc
 */

/*
 * Example 1: Behavior of the Tail Reader before getting the full history-size samples
 */
void example1(const SignalConfigPtr& signal)
{
    auto reader = TailReader(signal, 5);
    assert(reader.getAvailableCount() == 0u);

    // Allocate the buffers for the reader to copy data into
    SizeT count{};
    double values[5]{};
    Int domain[5]{};

    // Is below the history-size
    count = 3;
    reader.readWithDomain(values, domain, &count);
    assert(count == 0);

    try
    {
        // Is more than the history-size
        count = 6;
        reader.readWithDomain(values, domain, &count);
    }
    catch (const SizeTooLargeException& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // The Signal produces 3 samples: 1, 2, 3
    auto packet = createPacketForSignal(signal, 3);
    auto data = static_cast<double*>(packet.getData());
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    signal.sendPacket(packet);

    count = 5;
    reader.readWithDomain(values, domain, &count);

    // count = 3, values = { 1, 2, 3, 0, 0 }
    assert(count == 3u);
    assert(values[0] == 1);
    assert(values[1] == 2);
    assert(values[2] == 3);
    assert(values[3] == 0);
    assert(values[4] == 0);
}

/*
 * Example 2: Subsequent reads can have overlapping samples
 */
void example2(const SignalConfigPtr& signal)
{
    auto reader = TailReader(signal, 5);

    // The Signal produces 3 samples: 1, 2, 3
    const SizeT FIRST_PACKET_SAMPLES = 3u;
    auto packet = createPacketForSignal(signal, FIRST_PACKET_SAMPLES);
    auto data = static_cast<double*>(packet.getData());
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    signal.sendPacket(packet);

    // Allocate the buffers for the reader to copy data into
    SizeT count{5};
    double values[5]{};
    reader.read(values, &count);

    // count = 3, values = { 1, 2, 3, 0, 0 }
    assert(count == 3u);
    assert(values[0] == 1);
    assert(values[1] == 2);
    assert(values[2] == 3);
    assert(values[3] == 0);
    assert(values[4] == 0);

    // The Signal produces 3 samples: 4, 5, 6
    auto packet2 = createPacketForSignal(signal, 3, FIRST_PACKET_SAMPLES);
    auto data2 = static_cast<double*>(packet2.getData());
    data2[0] = 4;
    data2[1] = 5;
    data2[2] = 6;
    signal.sendPacket(packet2);

    count = 5;
    reader.read(values, &count);

    // count = 5, values = { 2, 3, 4, 5, 6 }
    assert(count == 5);
    assert(values[0] == 2);
    assert(values[1] == 3);
    assert(values[2] == 4);
    assert(values[3] == 5);
    assert(values[4] == 6);
}

void example3(const SignalConfigPtr& signal)
{
    auto reader = TailReader(signal, 5);

    /*
     * The Signal produces 3 Packets
     * [Packet 1]: 4 samples
     * [Packet 2]: 3 samples
     * [Packet 3]: 1 sample
     * -------------------------------
     *      Total: 8 samples
     */

    auto packet1 = createPacketForSignal(signal, 4);
    auto packet2 = createPacketForSignal(signal, 3);
    auto packet3 = createPacketForSignal(signal, 1);
    signal.sendPacket(packet1);
    signal.sendPacket(packet2);
    signal.sendPacket(packet3);

    assert(reader.getAvailableCount() == 8u);

    // Allocate the buffers for the reader to copy data into
    SizeT count{};
    double values[10]{};

    try
    {
        count = 10;

        // Will throw a SizeTooLargeException
        reader.read(values, &count);
    }
    catch (const SizeTooLargeException& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // Will succeed as [Packet 3] and [Packet 2] together are less than 5 samples,
    // and we still need to keep [Packet 1] around to satisfy the minimum of 5 samples
    count = 8;
    reader.read(values, &count);

    assert(count == 8u);
}

/*
 * ENTRY POINT
 */
int main(int /*argc*/, const char* /*argv*/ [])
{
    SignalConfigPtr signal = setupExampleSignal();

    example1(signal);
    example2(signal);
    example3(signal);

    return 0;
}

/*
 * Set up the Signal with Float64 data
 */
SignalConfigPtr setupExampleSignal()
{
    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, nullptr, nullptr);

    auto signal = Signal(context, nullptr, "example signal");
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    return signal;
}

DataDescriptorPtr setupDescriptor(daq::SampleType type, daq::DataRulePtr rule)
{
    // Set up the data descriptor with the provided Sample-Type
    const auto dataDescriptor = daq::DataDescriptorBuilder().setSampleType(type);

    // For the Domain, we provide a Linear Rule to generate time-stamps
    if (rule.assigned())
        dataDescriptor.setRule(rule);

    return dataDescriptor.build();
}

DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples, Int offset)
{
    // Create a Data Packet where the values are generated via the +1 rule starting at 0
    auto domainPacket = daq::DataPacket(
        setupDescriptor(daq::SampleType::Int64, daq::LinearDataRule(1, 0)),
        numSamples,
        offset // offset from 0 to start the sample generation at
    );

    return daq::DataPacketWithDomain(
        domainPacket,
        signal.getDescriptor(),
        numSamples
    );
}
