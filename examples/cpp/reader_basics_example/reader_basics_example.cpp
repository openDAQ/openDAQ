#include <opendaq/context_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_exceptions.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>

#include <cassert>
#include <iostream>

using namespace daq;

SignalConfigPtr setupExampleSignal();
SignalPtr setupExampleDomain(const SignalPtr& value);
DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples, Int offset = 0);
DataDescriptorPtr setupDescriptor(SampleType type, const DataRulePtr& rule = nullptr);

/*
 * Corresponding document: Antora/modules/howto_guides/pages/howto_read_with_domain.adoc
 */

/*
 * Example 1: These calls all create the same Reader
 */
void example1(const SignalConfigPtr& signal)
{
    auto reader1 = StreamReader(signal);
    auto reader2 = StreamReader<double, Int>(signal);
    auto reader3 = StreamReader(signal, SampleType::Float64, SampleType::Int64);

    // For value
    assert(reader1.getValueReadType() == SampleType::Float64);
    assert(reader2.getValueReadType() == SampleType::Float64);
    assert(reader3.getValueReadType() == SampleType::Float64);

    // For Domain
    assert(reader1.getDomainReadType() == SampleType::Int64);
    assert(reader2.getDomainReadType() == SampleType::Int64);
    assert(reader3.getDomainReadType() == SampleType::Int64);
}

/*
 * Example 2: Creating a Reader with the Signal’s Sample Type
 */
void example2(const SignalConfigPtr& signal)
{
    // Use the Signal's Sample Types for both value and Domain
    auto reader1 = StreamReader(signal, SampleType::Undefined, SampleType::Undefined);
    assert(reader1.getValueReadType() == SampleType::Float64);
    assert(reader1.getDomainReadType() == SampleType::Int64);

    // Only for value
    auto reader2 = StreamReader(signal, SampleType::Undefined, SampleType::Int64);
    assert(reader2.getValueReadType() == SampleType::Float64);
    assert(reader2.getDomainReadType() == SampleType::Int64);

    // Or only for Domain
    auto reader3 = StreamReader(signal, SampleType::Float64, SampleType::Undefined);
    assert(reader3.getValueReadType() == SampleType::Float64);
    assert(reader3.getDomainReadType() == SampleType::Int64);
}

/*
 * Example 3: Reading basic value and Domain data
 */
void example3(const SignalConfigPtr& signal)
{
    auto reader = StreamReader<double, Int>(signal);

    // Should return 0
    [[maybe_unused]] auto available = reader.getAvailableCount();
    assert(available == 0u);

    //
    // Signal produces 8 samples
    //
    auto packet1 = createPacketForSignal(signal, 8);
    signal.sendPacket(packet1);

    // Should return 8
    available = reader.getAvailableCount();
    assert(available == 8u);

    SizeT readCount{5};
    double values[5]{};
    reader.read(values, &readCount);

    std::cout << "Read " << readCount << " values" << std::endl;
    for (double value : values)
    {
        std::cout << value << std::endl;
    }

    readCount = 5;
    double newValues[5];
    Int newDomain[5];
    reader.readWithDomain(newValues, newDomain, &readCount);

    // `readCount` should now be 3
    std::cout << "Read another " << readCount << " value and Domain samples" << std::endl;
    for (SizeT i = 0; i < readCount; ++i)
    {
        std::cout << newValues[i] << ", " << newDomain[i] << std::endl;
    }
}

/*
 * Example 4: Handling Signal changes
 */
void example4(const SignalConfigPtr& signal)
{
    // Signal Sample Type value is `Float64`
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = StreamReader<double, Int>(signal);

    // Signal produces 2 samples { 1.1, 2.2 }
    auto packet1 = createPacketForSignal(signal, 2);
    auto data1 = static_cast<double*>(packet1.getData());
    data1[0] = 1.1;
    data1[1] = 2.2;

    signal.sendPacket(packet1);

    //
    // The value Sample Type of the `signal` changes from `Float64` to `Int32`
    //
    signal.setDescriptor(setupDescriptor(SampleType::Int32));

    // Signal produces 2 samples { 3, 4 }
    auto packet2 = createPacketForSignal(signal, 2);
    auto data2 = static_cast<std::int32_t*>(packet2.getData());
    data2[0] = 3;
    data2[1] = 4;

    signal.sendPacket(packet2);

    // If Descriptor has changed, Reader will return Reader status with that event
    // Call succeeds and results in 2 samples { 1.1, 2.2 }
    SizeT count{5};
    double values[5]{};
    auto status = reader.read(values, &count);
    assert(status.getReadStatus() == ReadStatus::Event);

    assert(count == 2u);
    assert(values[0] == 1.1);
    assert(values[1] == 2.2);

    // The subsequent call succeeds because `Int32` is convertible to `Float64`
    // and results in 2 samples { 3.0, 4.0 }
    reader.read(values, &count);
    assert(count == 2u);
    assert(values[0] == 3.0);
    assert(values[1] == 4.0);

    //
    // The value Sample Type of the `signal` changes from `Int32` to `Int64`
    //
    signal.setDescriptor(setupDescriptor(SampleType::Int64));

    // Signal produces 2 samples { 5, 6 }
    auto packet3 = createPacketForSignal(signal, 2);
    auto data3 = static_cast<std::int64_t*>(packet3.getData());
    data3[0] = 3;
    data3[1] = 4;
    signal.sendPacket(packet3);

    // Reader reads 0 values and returns status with new Event Packet
    SizeT newCount{2};
    double newValues[2]{};
    auto newStatus = reader.read(newValues, &newCount);
    assert(newCount == 0u);
    assert(newStatus.getReadStatus() == ReadStatus::Event);
}

/*
 * Example 5: Reader reuse
 */
void example5(const SignalConfigPtr& signal)
{
    signal.setDescriptor(setupDescriptor(SampleType::Int64));

    auto reader = StreamReader<Int, Int>(signal);

    // Signal produces 5 samples { 1, 2, 3, 4, 5 }
    auto packet1 = createPacketForSignal(signal, 5);
    auto data1 = static_cast<Int*>(packet1.getData());
    data1[0] = 1;
    data1[1] = 2;
    data1[2] = 3;
    data1[3] = 4;
    data1[4] = 5;

    signal.sendPacket(packet1);

    SizeT count{2};
    Int values[2]{};
    reader.read(values, &count);  // count = 2, values = { 1, 2 }

    assert(count == 2u);
    assert(values[0] == 1);
    assert(values[1] == 2);

    // Reuse the Reader
    auto newReader = StreamReaderFromExisting<double, Int>(reader);

    // New Reader successfully continues on from previous Reader's position
    count = 2;
    double newValues[2]{};
    newReader.read(newValues, &count);  // count = 2, values = { 3.0, 4.0 }

    assert(count == 2u);
    assert(newValues[0] == 3.0);
    assert(newValues[1] == 4.0);

    // The old Reader has been invalidated when reused by a new one
    count = 2;
    Int oldValues[2]{};
    auto status = reader.read(oldValues, &count);
    assert(status.getValid() == false);
}

/*
 * ENTRY POINT
 */
int main(int /*argc*/, const char* /*argv*/[])
{
    SignalConfigPtr signal = setupExampleSignal();
    signal.setDomainSignal(setupExampleDomain(signal));

    example1(signal);
    example2(signal);
    example3(signal);
    example4(signal);
    example5(signal);

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

SignalPtr setupExampleDomain(const SignalPtr& value)
{
    auto domain = Signal(value.getContext(), nullptr, "domain signal");
    domain.setDescriptor(setupDescriptor(SampleType::Int64, LinearDataRule(1, 0)));

    return domain;
}

DataDescriptorPtr setupDescriptor(SampleType type, const DataRulePtr& rule)
{
    // Set up the data descriptor with the provided Sample Type
    const auto dataDescriptor = DataDescriptorBuilder().setSampleType(type);

    // For the Domain, we provide a Linear Rule to generate time-stamps
    if (rule.assigned())
        dataDescriptor.setRule(rule);

    return dataDescriptor.build();
}

DataPacketPtr createPacketForSignal(const SignalPtr& signal, SizeT numSamples, Int offset)
{
    // Create a data packet where the values are generated via the +1 rule starting at 0
    auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(),
                                   numSamples,
                                   offset  // offset from 0 to start the sample generation at
    );

    return DataPacketWithDomain(domainPacket, signal.getDescriptor(), numSamples);
}
