#include <opendaq/signal_exceptions.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <gtest/gtest.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>

using DataDescriptorTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DataDescriptorTest, ValueDescriptorSetGet)
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

TEST_F(DataDescriptorTest, ValueDescriptorCopyFactory)
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

TEST_F(DataDescriptorTest, RuleScalingInteraction)
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

TEST_F(DataDescriptorTest, InvalidRuleSampleType)
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

TEST_F(DataDescriptorTest, ScalingTypeMismatch)
{
    auto linearScaling = LinearScaling(10, 10);
    auto explicitRule = ExplicitDataRule();
    auto desc = DataDescriptorBuilder().setSampleType(SampleType::Int16).setPostScaling(linearScaling).setRule(explicitRule);

    ASSERT_THROW(desc.build(), InvalidSampleTypeException);

    desc.setSampleType(SampleType::Float64);
    ASSERT_NO_THROW(desc.build());
}

TEST_F(DataDescriptorTest, SerializeDeserialize)
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

TEST_F(DataDescriptorTest, StructType)
{
    const auto structType = DataDescriptorStructType();
    const daq::StructPtr structPtr = DataDescriptorBuilder().build();
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(DataDescriptorTest, StructFields)
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

TEST_F(DataDescriptorTest, StructNames)
{
    const auto structType = DataDescriptorStructType();
    const daq::StructPtr structPtr = DataDescriptorBuilder().build();
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(DataDescriptorTest, DataDescriptorBuilderSetGet)
{
    auto dimensions = List<IDimension>(Dimension(LinearDimensionRule(10, 10, 10)));
    auto linearScaling = LinearScaling(10, 10);
    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";
    const auto dataDescriptorBuilder = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Float64)
                                        .setValueRange(Range(10, 1000))
                                        .setDimensions(dimensions)
                                        .setOrigin("testRef")
                                        .setTickResolution(Ratio(1, 1000))
                                        .setUnit(Unit("s", 10))
                                        .setRule(LinearDataRule(10, 10))
                                        .setName("testName")
                                        .setPostScaling(linearScaling)
                                        .setMetadata(metaData);
    
    ASSERT_EQ(dataDescriptorBuilder.getSampleType(), SampleType::Float64);
    ASSERT_EQ(dataDescriptorBuilder.getValueRange(), Range(10, 1000));
    ASSERT_EQ(dataDescriptorBuilder.getDimensions(), dimensions);
    ASSERT_EQ(dataDescriptorBuilder.getOrigin(), "testRef");
    ASSERT_EQ(dataDescriptorBuilder.getTickResolution(), Ratio(1, 1000));
    ASSERT_EQ(dataDescriptorBuilder.getUnit(), Unit("s", 10));
    ASSERT_EQ(dataDescriptorBuilder.getRule(), LinearDataRule(10, 10));
    ASSERT_EQ(dataDescriptorBuilder.getName(), "testName");
    ASSERT_EQ(dataDescriptorBuilder.getPostScaling(), linearScaling);
    ASSERT_EQ(dataDescriptorBuilder.getMetadata(), metaData);
}

TEST_F(DataDescriptorTest, DataDescriptorCreateFactory)
{
    auto dimensions = List<IDimension>(Dimension(LinearDimensionRule(10, 10, 10)));
    auto linearScaling = LinearScaling(10, 10);
    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";
    const auto dataDescriptorBuilder = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Float64)
                                        .setValueRange(Range(10, 1000))
                                        .setDimensions(dimensions)
                                        .setOrigin("testRef")
                                        .setTickResolution(Ratio(1, 1000))
                                        .setUnit(Unit("s", 10))
                                        .setRule(LinearDataRule(10, 10))
                                        .setName("testName")
                                        .setMetadata(metaData);
    const auto dataDescriptor = DataDescriptorFromBuilder(dataDescriptorBuilder);

    ASSERT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    ASSERT_EQ(dataDescriptor.getValueRange(), Range(10, 1000));
    ASSERT_EQ(dataDescriptor.getDimensions(), dimensions);
    ASSERT_EQ(dataDescriptor.getOrigin(), "testRef");
    ASSERT_EQ(dataDescriptor.getTickResolution(), Ratio(1, 1000));
    ASSERT_EQ(dataDescriptor.getUnit(), Unit("s", 10));
    ASSERT_EQ(dataDescriptor.getRule(), LinearDataRule(10, 10));
    ASSERT_EQ(dataDescriptor.getName(), "testName");
    ASSERT_EQ(dataDescriptor.getMetadata(), metaData);
}


END_NAMESPACE_OPENDAQ
