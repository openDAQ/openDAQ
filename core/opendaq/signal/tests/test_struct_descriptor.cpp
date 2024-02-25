#include <opendaq/signal_exceptions.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/dimension_rule_factory.h>
#include <gtest/gtest.h>

using StructDescriptorTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(StructDescriptorTest, StructDescriptorSetGet)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    ListPtr<IDataDescriptor> structMembers = List<IDataDescriptor>(DataDescriptorBuilder().build(), DataDescriptorBuilder().build());

    auto descriptor = DataDescriptorBuilder()
                      .setSampleType(SampleType::Struct)
                      .setDimensions(dimensions)
                      .setStructFields(structMembers)
                      .setName("testName")
                      .build();
    
    ASSERT_EQ(descriptor.getDimensions().getCount(), static_cast<SizeT>(3));
    ASSERT_EQ(descriptor.getStructFields().getCount(), static_cast<SizeT>(2));
    ASSERT_EQ(descriptor.getName(), "testName");
}

TEST_F(StructDescriptorTest, StructDescriptorCopyFactory)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    ListPtr<IDataDescriptor> structMembers = List<IDataDescriptor>(DataDescriptorBuilder().build(), DataDescriptorBuilder().build());

    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Struct)
                          .setDimensions(dimensions)
                          .setStructFields(structMembers)
                          .setName("testName")
                          .build();

    auto copy = DataDescriptorBuilderCopy(descriptor).build();
    ASSERT_EQ(copy.getDimensions().getCount(), static_cast<SizeT>(3));
    ASSERT_EQ(copy.getStructFields().getCount(), static_cast<SizeT>(2));
    ASSERT_EQ(copy.getName(), "testName");
}

TEST_F(StructDescriptorTest, SerializeDeserialize)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    ListPtr<IDataDescriptor> structMembers = List<IDataDescriptor>(DataDescriptorBuilder().build(), DataDescriptorBuilder().build());

    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Struct)
                          .setDimensions(dimensions)
                          .setStructFields(structMembers)
                          .setName("testName")
                          .build();

    auto serializer = JsonSerializer(False);
    descriptor.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto descriptor1 = deserializer.deserialize(serialized.toStdString()).asPtr<IDataDescriptor>();

    ASSERT_EQ(descriptor, descriptor1);
}

END_NAMESPACE_OPENDAQ
