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

    // Get last value of a Data Packet
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

    // The Data Descriptor for SampleType::RangeInt64
    auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::RangeInt64).build();
    // Create a Data Packet
    auto packet = DataPacket(descriptor, 5);

    // END DOCS CODE

    auto data = static_cast<int64_t*>(packet.getData());
    data[8] = 8;
    data[9] = 9;
    signal.sendPacket(packet);
    testing::internal::CaptureStdout();

    // START DOCS CODE

    // Get last value of a Signal
    auto lastValue = signal.getLastValue();

    // Print last value
    std::cout << "last value: " << lastValue << std::endl;

    // Cast to IRange
    auto range = lastValue.asPtr<IRange>();

    // Extract values
    auto low = range.getLowValue();
    auto high = range.getHighValue();

    // END DOCS CODE
    std::cout << "range high value: " << range.getHighValue() << std::endl;
    std::string output = testing::internal::GetCapturedStdout();
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalComplexFloat32)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // START DOCS CODE

    // The Data Descriptor for SampleType::ComplexFloat32
    auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::ComplexFloat32).build();
    // Create a Data Packet
    auto packet = DataPacket(descriptor, 5);

    // END DOCS CODE

    auto data = static_cast<float*>(packet.getData());
    data[8] = 8.1f;
    data[9] = 9.1f;

    signal.sendPacket(packet);

    // START DOCS CODE

    // Get last value of a Signal
    auto lastValue = signal.getLastValue();
    // Cast to ComplexNumberPtr
    auto complex = lastValue.asPtr<IComplexNumber>();
    // Extract values
    auto real = complex.getReal();
    auto imaginary = complex.getImaginary();

    // END DOCS CODE

    ASSERT_FLOAT_EQ(real, 8.1f);
    ASSERT_FLOAT_EQ(imaginary, 9.1f);
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalStruct)
{
    // Create Signal
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // START DOCS CODE

    // Create a Data Descriptor
    auto descriptor = DataDescriptorBuilder()
                          .setName("MyStruct")
                          .setSampleType(SampleType::Struct)
                          .setStructFields(List<DataDescriptorPtr>(
                              DataDescriptorBuilder().setName("MyInt32").setSampleType(SampleType::Int32).build(),
                              DataDescriptorBuilder().setName("MyFloat64").setSampleType(SampleType::Float64).build()))
                          .build();
    // Set the Data Descriptor, thereby adding MyStruct to the Type Manager
    signal.setDescriptor(descriptor);
    // Create a Data Packet
    auto packet = DataPacket(descriptor, 5);

    // END DOCS CODE

    // Prepare data packet
    auto sizeInBytes = sizeof(int32_t) + sizeof(double);
    auto data = packet.getData();
    auto start = static_cast<char*>(data);
    void* a = start + sizeInBytes * 4;
    auto A = static_cast<int32_t*>(a);
    *A = 12;
    void* b = start + sizeInBytes * 4 + sizeof(int32_t);
    auto B = static_cast<double*>(b);
    *B = 15.1;
    signal.sendPacket(packet);

    // START DOCS CODE

    // Get last value of a Signal
    auto lastValue = signal.getLastValue();
    // Cast to StructPtr
    auto myStruct = lastValue.asPtr<IStruct>();
    // Extract both values
    auto myInt = myStruct.get("MyInt32");
    auto myFloat = myStruct.get("MyFloat64");

    // END DOCS CODE

    // Check first member
    ASSERT_EQ(myInt, 12);

    // Check second member
    ASSERT_DOUBLE_EQ(myFloat, 15.1);
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalListOfInt)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);
    numbers.pushBack(3);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).setDimensions(dimensions).build();

    auto packet = DataPacket(descriptor, 5);
    int64_t* data = static_cast<int64_t*>(packet.getData());
    data[12] = 1;
    data[13] = 4;
    data[14] = 44;

    signal.sendPacket(packet);

    // START DOCS CODE

    // Get last value of a Signal
    auto lastValue = signal.getLastValue();
    // Cast to ListPtr
    auto myList = lastValue.asPtr<IList>();
    // Extract the third item on myList
    auto third = myList.getItemAt(2);

    // END DOCS CODE

    ASSERT_EQ(myList.getItemAt(0), 1);
    ASSERT_EQ(myList.getItemAt(1), 4);
    ASSERT_EQ(myList.getItemAt(2), 44);
}

END_NAMESPACE_OPENDAQ
