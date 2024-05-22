#include <opendaq/signal_exceptions.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <gtest/gtest.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/scaling_calc_private.h>
#include <opendaq/data_rule_calc_private.h>

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


TEST_F(DataDescriptorTest, DataDescriptorSampleSizeSimple)
{
    const auto dataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    ASSERT_EQ(dataDescriptor.getSampleSize(), 8u);
    ASSERT_EQ(dataDescriptor.getRawSampleSize(), 8u);
}

TEST_F(DataDescriptorTest, DataDescriptorSampleSizeSimplePostScaling)
{
    const auto dataDescriptor = DataDescriptorBuilder()
        .setSampleType(SampleType::Float64)
        .setPostScaling(LinearScaling(1, 0, SampleType::Int32, ScaledSampleType::Float64))
        .build();

    ASSERT_EQ(dataDescriptor.getSampleSize(), 8u);
    ASSERT_EQ(dataDescriptor.getRawSampleSize(), 4u);
}

TEST_F(DataDescriptorTest, DataDescriptorSampleSizeImplicitSimple)
{
    const auto dataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).setRule(LinearDataRule(10, 10)).build();

    ASSERT_EQ(dataDescriptor.getSampleSize(), 8u);
    ASSERT_EQ(dataDescriptor.getRawSampleSize(), 0u);
}

/*
*  Sample type represent C++ structured type
*
* struct CanMessage
* {
*     uint32_t arbId;
*     uint8_t length;
*     uint8_t data[64];
* }
*/
TEST_F(DataDescriptorTest, DataDescriptorSampleSizeStruct)
{
    const auto arbIdDescriptor = DataDescriptorBuilder()
        .setName("ArbId")
        .setSampleType(SampleType::UInt32)
        .build();

    const auto lengthDescriptor = DataDescriptorBuilder()
        .setName("Length")
        .setSampleType(SampleType::UInt8)
        .build();

    const auto dataDescriptor = DataDescriptorBuilder()
        .setName("Data")
        .setSampleType(SampleType::UInt8)
        .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).build()))
        .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
        .setName("Struct")
        .setSampleType(SampleType::Struct)
        .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
        .build();

    ASSERT_EQ(canMsgDescriptor.getSampleSize(), 69u);
    ASSERT_EQ(canMsgDescriptor.getRawSampleSize(), 69u);
}

TEST_F(DataDescriptorTest, DataDescriptorSampleSizeMixedStruct)
{
    const auto arbIdDescriptor =
        DataDescriptorBuilder().setName("ArbId").setSampleType(SampleType::UInt32).setRule(ExplicitDataRule()).build();

    const auto lengthDescriptor =
        DataDescriptorBuilder().setName("Length").setSampleType(SampleType::UInt8).setRule(LinearDataRule(10, 10)).build();

    const auto dataDescriptor = DataDescriptorBuilder()
                                    .setName("Data")
                                    .setSampleType(SampleType::UInt8)
                                    .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).build()))
                                    .setRule(ExplicitDataRule())
                                    .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
        .setName("Struct")
        .setSampleType(SampleType::Struct)
        .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
        .build();

    ASSERT_EQ(canMsgDescriptor.getSampleSize(), 69u);
    ASSERT_EQ(canMsgDescriptor.getRawSampleSize(), 68u);
}

TEST_F(DataDescriptorTest, QueryInterface)
{
    auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).setRule(LinearDataRule(10, 10)).build();

    auto desc1 = desc.asPtr<IDataDescriptor>();
    ASSERT_EQ(desc1.getSampleType(), SampleType::Float64);

    auto desc2 = desc.asPtr<IScalingCalcPrivate>();
    ASSERT_FALSE(desc2->hasScalingCalc());

    auto desc3 = desc.asPtr<IDataRuleCalcPrivate>();
    ASSERT_TRUE(desc3->hasDataRuleCalc());

    auto desc11 = desc.asPtr<IDataDescriptor>(true);
    ASSERT_EQ(desc11.getSampleType(), SampleType::Float64);

    auto desc12 = desc.asPtr<IScalingCalcPrivate>(true);
    ASSERT_FALSE(desc12->hasScalingCalc());

    auto desc13 = desc.asPtr<IDataRuleCalcPrivate>(true);
    ASSERT_TRUE(desc13->hasDataRuleCalc());
}

END_NAMESPACE_OPENDAQ
