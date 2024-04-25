#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using HowToGetLastValue = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalFloat64)
{
    const auto mySignal = Signal(NullContext(), nullptr, "sig");

    // The Data Descriptor for Float64
    auto myDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    auto dataPacket = DataPacket(myDescriptor, 5);
    auto data = static_cast<double*>(dataPacket.getData());
    data[4] = 6.66;
    mySignal.sendPacket(dataPacket);

    // START DOCS CODE

    // Get last value of a Signal
    auto myLastValue = mySignal.getLastValue();

    // END DOCS CODE

    // Cast to IFloat
    auto number = myLastValue.asPtr<IFloat>();

    // Assert equality
    ASSERT_EQ(number, 6.66);
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalStruct)
{
    // Create Signal
    const auto mySignal = Signal(NullContext(), nullptr, "sig");

    // Create a Data Descriptor
    auto descriptor = DataDescriptorBuilder()
                          .setName("MyStruct")
                          .setSampleType(SampleType::Struct)
                          .setStructFields(List<DataDescriptorPtr>(
                              DataDescriptorBuilder().setName("MyInt32").setSampleType(SampleType::Int32).build(),
                              DataDescriptorBuilder().setName("MyFloat64").setSampleType(SampleType::Float64).build()))
                          .build();
    // Set the Data Descriptor, thereby adding MyStruct to the Type Manager
    mySignal.setDescriptor(descriptor);
    // Create a Data Packet
    auto packet = DataPacket(descriptor, 5);

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
    mySignal.sendPacket(packet);

    // START DOCS CODE

    // Check Signal Data Descriptor's Sample Type and name
    auto myDescriptor = mySignal.getDescriptor();
    assert(myDescriptor.getSampleType() == SampleType::Struct);
    assert(myDescriptor.getName() == "MyStruct");
    // Check Struct Fields
    auto myStructFields = myDescriptor.getStructFields();
    assert(myStructFields.getCount() == 2);
    assert(myStructFields[0].getSampleType() == SampleType::Int32);
    assert(myStructFields[0].getName() == "MyInt32");
    assert(myStructFields[1].getSampleType() == SampleType::Float64);
    assert(myStructFields[1].getName() == "MyFloat64");

    // Get last value of a Signal
    StructPtr myStruct = mySignal.getLastValue();
    // Extract values
    int32_t myInt = myStruct.get("MyInt32");
    double myFloat = myStruct.get("MyFloat64");

    // END DOCS CODE

    // Check first member
    ASSERT_EQ(myInt, 12);

    // Check second member
    ASSERT_DOUBLE_EQ(myFloat, 15.1);
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalListOfInt)
{
    const auto mySignal = Signal(NullContext(), nullptr, "sig");

    auto numbers = List<INumber>();
    numbers.pushBack(1);
    numbers.pushBack(2);

    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(ListDimensionRule(numbers)));

    auto descriptor = DataDescriptorBuilder().setName("test").setSampleType(SampleType::Int64).setDimensions(dimensions).build();
    mySignal.setDescriptor(descriptor);

    auto packet = DataPacket(descriptor, 5);
    int64_t* data = static_cast<int64_t*>(packet.getData());
    data[8] = 4;
    data[9] = 44;

    mySignal.sendPacket(packet);

    // START DOCS CODE

    // Retrieve the Signal's Sample Type
    auto mySampleType = mySignal.getDescriptor().getSampleType();

    // Check Dimensions count in Signal's Data Descriptor
    assert(mySignal.getDescriptor().getDimensions().getCount() == 1);
    // Get last value of a Signal
    ListPtr<IBaseObject> myList = mySignal.getLastValue();
    // Check the number of elements in List
    assert(myList.getCount() == 2);
    // Extract the second item in List
    auto myItem = myList.getItemAt(1);

    // END DOCS CODE

    // Assert equality
    ASSERT_EQ(myList.getItemAt(0), 4);
    ASSERT_EQ(myList.getItemAt(1), 44);
    ASSERT_EQ(mySampleType, SampleType::Int64);
}

END_NAMESPACE_OPENDAQ
