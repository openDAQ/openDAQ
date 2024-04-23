#include <coreobjects/unit_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/time_reader.h>

#include <cassert>
#include <iostream>

using namespace daq;
using namespace date;

SignalConfigPtr setupExampleSignal();
SignalPtr setupExampleDomain(const SignalPtr& value);
DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples, Int offset = 0);
DataDescriptorPtr setupDescriptor(SampleType type, const DataRulePtr& rule = nullptr);
DataDescriptorBuilderPtr setupDescriptorBuilder(SampleType type, const DataRulePtr& rule = nullptr);

/*
 * Corresponding document: Antora/modules/howto_guides/pages/howto_read_with_timestamps.adoc
 */

/*
 * Example 1: Reading time with Time Reader
 */
void example1(const SignalConfigPtr& signal)
{
    auto reader = StreamReader(signal);

    // Signal produces 5 samples
    auto packet = createPacketForSignal(signal, 5);
    auto data = static_cast<double*>(packet.getData());
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;

    signal.sendPacket(packet);

    auto timeReader = TimeReader(reader);

    SizeT count{5};
    double values[5]{};
    std::chrono::system_clock::time_point timeStamps[5]{};

    // Read with Time Reader
    timeReader.readWithDomain(values, timeStamps, &count);
    assert(count == 5);

    for (SizeT i = 0u; i < count; ++i)
    {
        std::cout << timeStamps[i] << ": " << values[i] << std::endl;
        assert(values[i] == i + 1);
    }

    std::cout << std::endl;
}

/*
 * Example 2: Reading time with the wrapped Reader
 */
void example2(const SignalConfigPtr& signal)
{
    auto reader = StreamReader(signal);

    // Signal produces 5 samples
    auto packet = createPacketForSignal(signal, 5);
    auto data = static_cast<double*>(packet.getData());
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    signal.sendPacket(packet);

    auto timeReader = TimeReader(reader);

    SizeT count{5};
    double values[5]{};
    std::chrono::system_clock::time_point timeStamps[5]{};

    // Read with the wrapped Reader
    reader.readWithDomain(values, timeStamps, &count);
    assert(count == 5);

    for (SizeT i = 0u; i < count; ++i)
    {
        std::cout << timeStamps[i] << ": " << values[i] << std::endl;
        assert(values[i] == i + 1);
    }
}

/*
 * ENTRY POINT
 */
int main(int /*argc*/, const char* /*argv*/[])
{
    SignalConfigPtr signal = setupExampleSignal();
    signal.setDomainSignal(setupExampleDomain(signal));

    /*
      The output in both examples should be:

        2022-09-27 00:02:03.0000000: 1
        2022-09-27 00:02:03.0010000: 2
        2022-09-27 00:02:03.0020000: 3
        2022-09-27 00:02:03.0030000: 4
        2022-09-27 00:02:03.0040000: 5
     */

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

SignalPtr setupExampleDomain(const SignalPtr& value)
{
    auto domainDataDescriptor = setupDescriptorBuilder(SampleTypeFromType<ClockTick>::SampleType, LinearDataRule(1, 0))
                                    .setOrigin("2022-09-27T00:02:03+00:00")
                                    .setTickResolution(Ratio(1, 1000))
                                    .setUnit(Unit("s", -1, "seconds", "time"))
                                    .build();

    auto domain = Signal(value.getContext(), nullptr, "domain signal");
    domain.setDescriptor(domainDataDescriptor);

    return domain;
}

DataDescriptorPtr setupDescriptor(SampleType type, const DataRulePtr& rule)
{
    return setupDescriptorBuilder(type, rule).build();
}

DataDescriptorBuilderPtr setupDescriptorBuilder(SampleType type, const DataRulePtr& rule)
{
    // Set up the Data Descriptor with the provided Sample Type
    const auto dataDescriptor = DataDescriptorBuilder().setSampleType(type);

    // For the Domain, we provide a Linear Rule to generate time-stamps
    if (rule.assigned())
        dataDescriptor.setRule(rule);

    return dataDescriptor;
}

DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples, Int offset)
{
    // Create a Data Packet where the values are generated via the +1 rule starting at 0
    auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(),
                                        numSamples,
                                        offset  // offset from 0 to start the sample generation at
    );

    return DataPacketWithDomain(domainPacket, signal.getDescriptor(), numSamples);
}
