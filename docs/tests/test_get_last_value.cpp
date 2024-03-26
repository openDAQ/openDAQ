#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using HowToGetLastValue = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalInt64)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");
    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).build();
    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<int64_t*>(dataPacket.getData());
    data[4] = 4;
    signal.sendPacket(dataPacket);

    // START DOCS CODE

    auto lastValue = signal.getLastValue();

    // END DOCS CODE

    IntegerPtr integerPtr;
    ASSERT_NO_THROW(integerPtr = lastValue.asPtr<IInteger>());
    ASSERT_EQ(integerPtr, 4);
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueDataPacketInt64)
{
    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();
    const auto packet = DataPacket(descriptor, 5);
    const auto data = static_cast<int32_t*>(packet.getData());
    data[4] = 42;
    testing::internal::CaptureStdout();

    // START DOCS CODE

    // Get last value on Data Packet
    auto lastValue = packet.getLastValue();

    // END DOCS CODE

    std::cout << "lastValue: " << lastValue << std::endl;
    auto integer = lastValue.asPtr<IInteger>();
    std::cout << "integer: " << integer << std::endl;
    ASSERT_EQ(integer, 42);
    std::string output = testing::internal::GetCapturedStdout();
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalRange)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // START DOCS CODE

    // The descriptor for SampleType::RangeInt64
    auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::RangeInt64).build();
    // Create packet
    auto packet = DataPacket(descriptor, 5);

    // END DOCS CODE    
    auto data = static_cast<int64_t*>(packet.getData());
    data[8] = 8;
    data[9] = 9;
    signal.sendPacket(packet);
    testing::internal::CaptureStdout();

    // START DOCS CODE

    // Get last value on Signal
    auto lastValue = signal.getLastValue();

    // Print last value
    std::cout << "last value: " << lastValue << std::endl;

    // Cast to RangePtr
    auto range = lastValue.asPtr<IRange>();

    // Call some methods
    auto low = range.getLowValue();
    auto high = range.getHighValue();

    // END DOCS CODE
    std::cout << "range high value: " << range.getHighValue() << std::endl;
    std::string output = testing::internal::GetCapturedStdout();
}

END_NAMESPACE_OPENDAQ
