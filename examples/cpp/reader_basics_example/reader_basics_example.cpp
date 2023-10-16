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
DataDescriptorPtr setupDescriptor(daq::SampleType type, const daq::DataRulePtr& rule = nullptr);

/*
 * Example 1: These calls all create the same reader
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

    // For domain
    assert(reader1.getDomainReadType() == SampleType::Int64);
    assert(reader2.getDomainReadType() == SampleType::Int64);
    assert(reader3.getDomainReadType() == SampleType::Int64);
}

/*
 * Example 2: Creating a reader with the Signal’s sample-type
 */
void example2(const SignalConfigPtr& signal)
{
    // Use the Signal's sample-types for both value and domain
    auto reader1 = StreamReader(signal, SampleType::Undefined, SampleType::Undefined);
    assert(reader1.getValueReadType() == SampleType::Float64);
    assert(reader1.getDomainReadType() == SampleType::Int64);

    // ony for value
    auto reader2 = StreamReader(signal, SampleType::Undefined, SampleType::Int64);
    assert(reader2.getValueReadType() == SampleType::Float64);
    assert(reader2.getDomainReadType() == SampleType::Int64);

    // or ony for domain
    auto reader3 = StreamReader(signal, SampleType::Float64, SampleType::Undefined);
    assert(reader3.getValueReadType() == SampleType::Float64);
    assert(reader3.getDomainReadType() == SampleType::Int64);
}

/*
 * Reading basic value and domain data
 */
void example3(const SignalConfigPtr& signal)
{
    auto reader = StreamReader<double, Int>(signal);

    // Should return 0
    [[maybe_unused]]
    auto available = reader.getAvailableCount();
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

    // `readCount` should now be `3`
    std::cout << "Read another " << readCount << " value and domain samples" << std::endl;
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
    // Signal value sample-type is `Float64`
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = StreamReader<double, Int>(signal);

    // Signal produces 2 samples { 1.1, 2.2 }
    auto packet1 = createPacketForSignal(signal, 2);
    auto data1 = static_cast<double*>(packet1.getData());
    data1[0] = 1.1;
    data1[1] = 2.2;

    signal.sendPacket(packet1);

    //
    // The value sample-type of the `signal` changes from `Float64` to `Int32`
    //
    signal.setDescriptor(setupDescriptor(SampleType::Int32));

    // Signal produces 2 samples { 3, 4 }
    auto packet2 = createPacketForSignal(signal, 2);
    auto data2 = static_cast<std::int32_t*>(packet2.getData());
    data2[0] = 3;
    data2[1] = 4;

    signal.sendPacket(packet2);

    // The call succeeds because `Int32` is convertible to `Float64`
    // and results in `4` samples { 1.1, 2.2, 3.0, 4.0 }

    SizeT count{5};
    double values[5]{};
    reader.read(values, &count);

    assert(count == 4u);
    assert(values[0] == 1.1);
    assert(values[1] == 2.2);
    assert(values[2] == 3.0);
    assert(values[3] == 4.0);

    // Instal a custom callback that invalidates the reader if the new value sample-type is `Int64`
    reader.setOnDescriptorChanged([](const DataDescriptorPtr& valueDescriptor,
                                     const DataDescriptorPtr& /*domainDescriptor*/)
    {
        // If the value descriptor has changed
        if (valueDescriptor.assigned())
        {
            // and the new sample type is `Int64`
            if (valueDescriptor.getSampleType() == SampleType::Int64)
            {
                return false;
            }
        }

        return true;
    });

    //
    // The value sample-type of the `signal` changes from `Int32` to `Int64`
    //
    signal.setDescriptor(setupDescriptor(SampleType::Int64));

    // Signal produces 2 samples { 5, 6 }
    auto packet3 = createPacketForSignal(signal, 2);
    auto data3 = static_cast<std::int64_t*>(packet3.getData());
    data3[0] = 3;
    data3[1] = 4;
    signal.sendPacket(packet3);

    [[maybe_unused]]
    bool failed{true};
    try
    {
        count = {2};
        double newValues[2]{};

        // Fails even if the new sample-type is convertible to `double` because
        // the user callback invalidated the reader.
        reader.read(newValues, &count);

        failed = false;
    }
    catch (const InvalidDataException& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    assert(failed);
}

/*
 * Example 5: Reader invalidation
 */
void example5(const SignalConfigPtr& signal)
{
    // Signal value sample-type is `Float64`
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = StreamReader<double, Int>(signal);

    // Instal a custom callback that invalidates the reader if the new value sample-type is `Int64`
    reader.setOnDescriptorChanged([](const DataDescriptorPtr& valueDescriptor,
                                     const DataDescriptorPtr& /*domainDescriptor*/)
    {
        // If the value descriptor has changed
        if (valueDescriptor.assigned())
        {
            // and the new sample type is `Int64`
            if (valueDescriptor.getSampleType() == SampleType::Int16)
            {
                return false;
            }
        }

        return true;
    });

    //
    // The value sample-type of the `signal` changes from `Float64` to `Int16`
    //
    signal.setDescriptor(setupDescriptor(SampleType::Int16));

    //
    // Signal produces 2 samples { 1, 2 }
    //
    auto packet = createPacketForSignal(signal, 2);
    auto data = static_cast<std::int16_t*>(packet.getData());
    data[0] = 1;
    data[1] = 2;
    signal.sendPacket(packet);

    [[maybe_unused]]
    bool failed{true};
    try
    {
        // Fails even if the new sample-type is convertible to `double` because
        // the user callback invalidated the reader.

        SizeT count{5};
        double values[5]{};
        reader.read(values, &count);

        failed = false;
    }
    catch (const InvalidDataException& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    assert(failed);

    // Clear the user callback
    reader.setOnDescriptorChanged(nullptr);

    // This will reuse the Reader's configuration and Connection but change read type
    // from to `Float64` to `Int64` and clear the `invalid` state.
    auto newReader = StreamReaderFromExisting<Int, Int>(reader);

    SizeT count{5};
    Int values[5]{};
    newReader.read(values, &count); // count = 2, values = { 1, 2 }

    assert(count == 2u);
    assert(values[0] == 1);
    assert(values[1] == 2);
}

/*
 * Example 6: Reader reuse
 */
void example6(const SignalConfigPtr& signal)
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

    // Reuse the reader
    auto newReader = StreamReaderFromExisting<double, Int>(reader);

    // new reader successfully continues on from previous reader's position
    count = 2;
    double newValues[2]{};
    newReader.read(newValues, &count);  // count = 2, values = { 3, 4 }

    assert(count == 2u);
    assert(newValues[0] == 3);
    assert(newValues[1] == 4);

    [[maybe_unused]]
    bool failed{true};
    try
    {
        // The old reader has been invalidated when reused by a new one

        count = 2;
        Int oldValues[2]{};
        reader.read(oldValues, &count);

        failed = false;
    }
    catch (const InvalidDataException& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    assert(failed);
}

/*
 * ENTRY POINT
 */
int main(int /*argc*/, const char* /*argv*/ [])
{
    SignalConfigPtr signal = setupExampleSignal();
    signal.setDomainSignal(setupExampleDomain(signal));

    example1(signal);
    example2(signal);
    example3(signal);
    example4(signal);
    example5(signal);
    example6(signal);

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
    domain.setDescriptor(setupDescriptor(daq::SampleType::Int64, daq::LinearDataRule(1, 0)));

    return domain;
}

DataDescriptorPtr setupDescriptor(SampleType type, const daq::DataRulePtr& rule)
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
    // Create a data packet where the values are generated via the +1 rule starting at 0
    auto domainPacket = daq::DataPacket(
        signal.getDomainSignal().getDescriptor(),
        numSamples,
        offset  // offset from 0 to start the sample generation at
    );

    return daq::DataPacketWithDomain(
        domainPacket,
        signal.getDescriptor(),
        numSamples
    );
}
