#include <opendaq/signal_exceptions.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <gtest/gtest.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>

using ValueDescriptorTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ValueDescriptorTest, ValueDescriptorSetGet)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";

    auto descriptor = DataDescriptorBuilder()
                      .setSampleType(SampleType::Float64)
                      .setValueRange(Range(10, 11.5))
                      .setDimensions(dimensions)
                      .setOrigin("testRef")
                      .setTickResolution(Ratio(1, 1000))
                      .setUnit(Unit("s", 10))
                      .setRule(LinearDataRule(10, 10))
                      .setName("testName")
                      .setMetadata(metaData)
                      .build();


    ASSERT_EQ(descriptor.getValueRange().getLowValue(), 10);
    ASSERT_EQ(descriptor.getValueRange().getHighValue(), 11.5);
    ASSERT_EQ(descriptor.getSampleType(), SampleType::Float64);
    ASSERT_EQ(descriptor.getDimensions().getCount(), static_cast<SizeT>(3));
    ASSERT_TRUE(descriptor.getUnit().assigned());
    ASSERT_EQ(descriptor.getRule().getParameters().get("delta"), 10);
    ASSERT_EQ(descriptor.getName(), "testName");
    ASSERT_EQ(descriptor.getMetadata().get("key"), "value");
}

TEST_F(ValueDescriptorTest, ValueDescriptorCopyFactory)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";

    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Float64)
                          .setValueRange(Range(10, 11.5))
                          .setDimensions(dimensions)
                          .setOrigin("testRef")
                          .setTickResolution(Ratio(1, 1000))
                          .setUnit(Unit("s", 10))
                          .setRule(ExplicitDataRule())
                          .setName("testName")
                          .setMetadata(metaData)
                          .build();

    auto copy = DataDescriptorBuilderCopy(descriptor).build();
    ASSERT_TRUE(copy.getDimensions().isFrozen());
    ASSERT_TRUE(copy.getMetadata().isFrozen());

    ASSERT_EQ(copy.getValueRange().getLowValue(), 10);
    ASSERT_EQ(copy.getValueRange().getHighValue(), 11.5);
    ASSERT_EQ(copy.getSampleType(), SampleType::Float64);
    ASSERT_EQ(copy.getDimensions().getCount(), static_cast<SizeT>(3));
    ASSERT_TRUE(copy.getUnit().assigned());
    ASSERT_EQ(copy.getRule().getType(), DataRuleType::Explicit);
    ASSERT_EQ(copy.getName(), "testName");
    ASSERT_EQ(copy.getMetadata().get("key"), "value");
}

TEST_F(ValueDescriptorTest, RuleScalingInteraction)
{
    auto explicitRule = ExplicitDataRule();
    auto linearRule = LinearDataRule(10, 10);
    auto linearScaling = LinearScaling(10, 10);

    auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).setRule(linearRule).build();

    auto descBuilder = DataDescriptorBuilderCopy(desc).setPostScaling(linearScaling);
    ASSERT_THROW(descBuilder.build(), InvalidStateException);

    descBuilder.setRule(explicitRule);
    ASSERT_NO_THROW(descBuilder.build());
}

TEST_F(ValueDescriptorTest, InvalidRuleSampleType)
{
    auto linearDataRule = LinearDataRule(10, 10);
    auto explicitRule = ExplicitDataRule();
    auto desc = DataDescriptorBuilder().setSampleType(SampleType::ComplexFloat32).setRule(linearDataRule);

    ASSERT_THROW(desc.build(), InvalidSampleTypeException);
    desc.setSampleType(SampleType::Int64);

    ASSERT_NO_THROW(desc.build());

    desc = DataDescriptorBuilderCopy(desc.build()).setRule(explicitRule).setSampleType(SampleType::Binary);

    ASSERT_NO_THROW(desc.build());
}

TEST_F(ValueDescriptorTest, ScalingTypeMismatch)
{
    auto linearScaling = LinearScaling(10, 10);
    auto explicitRule = ExplicitDataRule();
    auto desc = DataDescriptorBuilder().setSampleType(SampleType::Int16).setPostScaling(linearScaling).setRule(explicitRule);

    ASSERT_THROW(desc.build(), InvalidSampleTypeException);

    desc.setSampleType(SampleType::Float64);
    ASSERT_NO_THROW(desc.build());
}

TEST_F(ValueDescriptorTest, SerializeDeserialize)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";

    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::UInt8)
                          .setValueRange(Range(10, 11.5))
                          .setDimensions(dimensions)
                          .setOrigin("testRef")
                          .setTickResolution(Ratio(1, 1000))
                          .setUnit(Unit("s", 10))
                          .setRule(LinearDataRule(10, 10))
                          .setName("testName")
                          .setMetadata(metaData)
                          .build();

    auto serializer = JsonSerializer(False);
    descriptor.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto descriptor1 = deserializer.deserialize(serialized.toStdString()).asPtr<IDataDescriptor>();

    ASSERT_EQ(descriptor, descriptor1);
}

TEST_F(ValueDescriptorTest, StructType)
{
    const auto structType = DataDescriptorStructType();
    const daq::StructPtr structPtr = DataDescriptorBuilder().build();
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(ValueDescriptorTest, StructFields)
{
    auto dimensions = List<IDimension>();
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));
    dimensions.pushBack(Dimension(LinearDimensionRule(10, 10, 10)));

    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";
    const StructPtr descriptor = DataDescriptorBuilder()
                      .setSampleType(SampleType::UInt8)
                      .setValueRange(Range(10, 11.5))
                      .setDimensions(dimensions)
                      .setOrigin("testRef")
                      .setTickResolution(Ratio(1, 1000))
                      .setUnit(Unit("s", 10))
                      .setRule(LinearDataRule(10, 10))
                      .setName("testName")
                      .setMetadata(metaData)
                      .build();

    ASSERT_EQ(descriptor.get("sampleType"), static_cast<Int>(SampleType::UInt8));
    ASSERT_EQ(descriptor.get("valueRange"), Range(10, 11.5));
    ASSERT_EQ(descriptor.get("dimensions"), dimensions);
    ASSERT_EQ(descriptor.get("origin"), "testRef");
    ASSERT_EQ(descriptor.get("tickResolution"), Ratio(1, 1000));
    ASSERT_EQ(descriptor.get("unit"), Unit("s", 10));
    ASSERT_EQ(descriptor.get("name"), "testName");
    ASSERT_EQ(descriptor.get("metadata"), metaData);
    ASSERT_EQ(descriptor.get("structField"), nullptr);
    ASSERT_EQ(descriptor.get("scaling"), nullptr);
    ASSERT_EQ(descriptor.get("dataRule"), LinearDataRule(10, 10));
}

TEST_F(ValueDescriptorTest, StructNames)
{
    const auto structType = DataDescriptorStructType();
    const daq::StructPtr structPtr = DataDescriptorBuilder().build();
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

END_NAMESPACE_OPENDAQ
