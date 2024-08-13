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
#include "testutils/testutils.h"

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
                      .setReferenceDomainId("testReferenceDomainId")
                      .setReferenceDomainOffset(53)
                      .setReferenceDomainIsAbsolute(False)
                      .build();


    ASSERT_EQ(descriptor.getValueRange().getLowValue(), 10);
    ASSERT_EQ(descriptor.getValueRange().getHighValue(), 11.5);
    ASSERT_EQ(descriptor.getSampleType(), SampleType::Float64);
    ASSERT_EQ(descriptor.getDimensions().getCount(), static_cast<SizeT>(3));
    ASSERT_TRUE(descriptor.getUnit().assigned());
    ASSERT_EQ(descriptor.getRule().getParameters().get("delta"), 10);
    ASSERT_EQ(descriptor.getName(), "testName");
    ASSERT_EQ(descriptor.getMetadata().get("key"), "value");
    ASSERT_EQ(descriptor.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(descriptor.getReferenceDomainOffset(), 53);
    ASSERT_EQ(descriptor.getReferenceDomainIsAbsolute(), False);
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
                          .setRule(LinearDataRule(1, 2))
                          .setName("testName")
                          .setMetadata(metaData)
                          .setReferenceDomainId("testReferenceDomainId")
                          .setReferenceDomainOffset(53)
                          .setReferenceDomainIsAbsolute(False)
                          .build();

    auto copy = DataDescriptorBuilderCopy(descriptor).build();
    ASSERT_TRUE(copy.getDimensions().isFrozen());
    ASSERT_TRUE(copy.getMetadata().isFrozen());

    ASSERT_EQ(copy.getValueRange().getLowValue(), 10);
    ASSERT_EQ(copy.getValueRange().getHighValue(), 11.5);
    ASSERT_EQ(copy.getSampleType(), SampleType::Float64);
    ASSERT_EQ(copy.getDimensions().getCount(), static_cast<SizeT>(3));
    ASSERT_TRUE(copy.getUnit().assigned());
    ASSERT_EQ(copy.getRule().getType(), DataRuleType::Linear);
    ASSERT_EQ(copy.getName(), "testName");
    ASSERT_EQ(copy.getMetadata().get("key"), "value");
    ASSERT_EQ(copy.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(copy.getReferenceDomainOffset(), 53);
    ASSERT_EQ(copy.getReferenceDomainIsAbsolute(), False);
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
                          .setReferenceDomainId("testReferenceDomainId")
                          .setReferenceDomainOffset(53)
                          .setReferenceDomainIsAbsolute(False)
                          .build();

    auto serializer = JsonSerializer(False);
    descriptor.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto descriptor1 = deserializer.deserialize(serialized.toStdString()).asPtr<IDataDescriptor>();

    ASSERT_EQ(descriptor, descriptor1);
}

TEST_F(DataDescriptorTest, DeserializeBackwardsCompat)
{
    // Without referenceDomainId/referenceDomainOffset/referenceDomainIsAbsolute

    std::string serialized = R"({"__type":"DataDescriptor","name":"testName","sampleType":3,"unit":{"__type":"Unit","symbol":"s","id":10,"name":"","quantity":""},"dimensions":[{"__type":"Dimension","rule":{"__type":"DimensionRule","rule_type":1,"params":{"__type":"Dict","values":[{"key":"delta","value":10},{"key":"start","value":10},{"key":"size","value":10}]}},"name":""},{"__type":"Dimension","rule":{"__type":"DimensionRule","rule_type":1,"params":{"__type":"Dict","values":[{"key":"delta","value":10},{"key":"start","value":10},{"key":"size","value":10}]}},"name":""},{"__type":"Dimension","rule":{"__type":"DimensionRule","rule_type":1,"params":{"__type":"Dict","values":[{"key":"delta","value":10},{"key":"start","value":10},{"key":"size","value":10}]}},"name":""}],"valueRange":{"__type":"Range","low":10,"high":11.5},"rule":{"__type":"DataRule","ruleType":1,"params":{"__type":"Dict","values":[{"key":"delta","value":10},{"key":"start","value":10}]}},"origin":"testRef","tickResolution":{"__type":"Ratio","num":1,"den":1000},"metadata":{"__type":"Dict","values":[{"key":"key","value":"value"}]},"structFields":[]})";

    auto deserializer = JsonDeserializer();
    DataDescriptorPtr descriptor;
    ASSERT_NO_THROW(descriptor = deserializer.deserialize(serialized));
    ASSERT_EQ(descriptor.getName(), "testName");
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
                      .setReferenceDomainId("testReferenceDomainId")
                      .setReferenceDomainOffset(53)
                      .setReferenceDomainIsAbsolute(False)
                      .build();


    ASSERT_EQ(descriptor.get("SampleType"), static_cast<Int>(SampleType::UInt8));
    ASSERT_EQ(descriptor.get("ValueRange"), Range(10, 11.5));
    ASSERT_EQ(descriptor.get("Dimensions"), dimensions);
    ASSERT_EQ(descriptor.get("Origin"), "testRef");
    ASSERT_EQ(descriptor.get("TickResolution"), Ratio(1, 1000));
    ASSERT_EQ(descriptor.get("Unit"), Unit("s", 10));
    ASSERT_EQ(descriptor.get("Name"), "testName");
    ASSERT_EQ(descriptor.get("Metadata"), metaData);
    ASSERT_EQ(descriptor.get("StructField"), nullptr);
    ASSERT_EQ(descriptor.get("Scaling"), nullptr);
    ASSERT_EQ(descriptor.get("DataRule"), LinearDataRule(10, 10));
    ASSERT_EQ(descriptor.get("ReferenceDomainId"), "testReferenceDomainId");
    ASSERT_EQ(descriptor.get("ReferenceDomainOffset"), 53);
    ASSERT_EQ(descriptor.get("ReferenceDomainIsAbsolute"), False);
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
    const auto descriptorBuilder = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Float64)
                                        .setValueRange(Range(10, 1000))
                                        .setDimensions(dimensions)
                                        .setOrigin("testRef")
                                        .setTickResolution(Ratio(1, 1000))
                                        .setUnit(Unit("s", 10))
                                        .setRule(LinearDataRule(10, 10))
                                        .setName("testName")
                                        .setPostScaling(linearScaling)
                                        .setMetadata(metaData)
                                        .setReferenceDomainId("testReferenceDomainId")
                                        .setReferenceDomainOffset(53)
                                        .setReferenceDomainIsAbsolute(False);
    
    ASSERT_EQ(descriptorBuilder.getSampleType(), SampleType::Float64);
    ASSERT_EQ(descriptorBuilder.getValueRange(), Range(10, 1000));
    ASSERT_EQ(descriptorBuilder.getDimensions(), dimensions);
    ASSERT_EQ(descriptorBuilder.getOrigin(), "testRef");
    ASSERT_EQ(descriptorBuilder.getTickResolution(), Ratio(1, 1000));
    ASSERT_EQ(descriptorBuilder.getUnit(), Unit("s", 10));
    ASSERT_EQ(descriptorBuilder.getRule(), LinearDataRule(10, 10));
    ASSERT_EQ(descriptorBuilder.getName(), "testName");
    ASSERT_EQ(descriptorBuilder.getPostScaling(), linearScaling);
    ASSERT_EQ(descriptorBuilder.getMetadata(), metaData);
    ASSERT_EQ(descriptorBuilder.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(descriptorBuilder.getReferenceDomainOffset(), 53);
    ASSERT_EQ(descriptorBuilder.getReferenceDomainIsAbsolute(), False);
}

TEST_F(DataDescriptorTest, DataDescriptorCreateFactory)
{
    auto dimensions = List<IDimension>(Dimension(LinearDimensionRule(10, 10, 10)));
    auto linearScaling = LinearScaling(10, 10);
    auto metaData = Dict<IString, IString>();
    metaData["key"] = "value";
    const auto descriptorBuilder = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Float64)
                                        .setValueRange(Range(10, 1000))
                                        .setDimensions(dimensions)
                                        .setOrigin("testRef")
                                        .setTickResolution(Ratio(1, 1000))
                                        .setUnit(Unit("s", 10))
                                        .setRule(LinearDataRule(10, 10))
                                        .setName("testName")
                                        .setMetadata(metaData)
                                        .setReferenceDomainId("testReferenceDomainId")
                                        .setReferenceDomainOffset(53)
                                        .setReferenceDomainIsAbsolute(False);

    const auto descriptor = DataDescriptorFromBuilder(descriptorBuilder);

    ASSERT_EQ(descriptor.getSampleType(), SampleType::Float64);
    ASSERT_EQ(descriptor.getValueRange(), Range(10, 1000));
    ASSERT_EQ(descriptor.getDimensions(), dimensions);
    ASSERT_EQ(descriptor.getOrigin(), "testRef");
    ASSERT_EQ(descriptor.getTickResolution(), Ratio(1, 1000));
    ASSERT_EQ(descriptor.getUnit(), Unit("s", 10));
    ASSERT_EQ(descriptor.getRule(), LinearDataRule(10, 10));
    ASSERT_EQ(descriptor.getName(), "testName");
    ASSERT_EQ(descriptor.getMetadata(), metaData);
    ASSERT_EQ(descriptor.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(descriptor.getReferenceDomainOffset(), 53);
    ASSERT_EQ(descriptor.getReferenceDomainIsAbsolute(), False);
}


TEST_F(DataDescriptorTest, DataDescriptorSampleSizeSimple)
{
    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    ASSERT_EQ(descriptor.getSampleSize(), 8u);
    ASSERT_EQ(descriptor.getRawSampleSize(), 8u);
}

TEST_F(DataDescriptorTest, DataDescriptorSampleSizeSimplePostScaling)
{
    const auto descriptor = DataDescriptorBuilder()
        .setSampleType(SampleType::Float64)
        .setPostScaling(LinearScaling(1, 0, SampleType::Int32, ScaledSampleType::Float64))
        .build();

    ASSERT_EQ(descriptor.getSampleSize(), 8u);
    ASSERT_EQ(descriptor.getRawSampleSize(), 4u);
}

TEST_F(DataDescriptorTest, DataDescriptorSampleSizeImplicitSimple)
{
    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).setRule(LinearDataRule(10, 10)).build();

    ASSERT_EQ(descriptor.getSampleSize(), 8u);
    ASSERT_EQ(descriptor.getRawSampleSize(), 0u);
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

    const auto descriptor = DataDescriptorBuilder()
        .setName("Data")
        .setSampleType(SampleType::UInt8)
        .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).build()))
        .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
        .setName("Struct")
        .setSampleType(SampleType::Struct)
        .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, descriptor))
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

    const auto descriptor = DataDescriptorBuilder()
                                    .setName("Data")
                                    .setSampleType(SampleType::UInt8)
                                    .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).build()))
                                    .setRule(ExplicitDataRule())
                                    .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
        .setName("Struct")
        .setSampleType(SampleType::Struct)
        .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, descriptor))
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

TEST_F(DataDescriptorTest, DisallowReferenceDomainIdForConstantDataRule)
{
    ASSERT_THROW_MSG(
        DataDescriptorBuilder().setSampleType(SampleType::Int32).setRule(ConstantDataRule()).setReferenceDomainId("RefDomId").build(),
        InvalidParameterException,
        "Reference domain id not supported for constant data rule type.");
}

TEST_F(DataDescriptorTest, DisallowReferenceDomainOffsetForConstantDataRule)
{
    ASSERT_THROW_MSG(
        DataDescriptorBuilder().setSampleType(SampleType::Int32).setRule(ConstantDataRule()).setReferenceDomainOffset(100).build(),
        InvalidParameterException,
        "Reference domain offset not supported for constant data rule type.");
}

TEST_F(DataDescriptorTest, DisallowReferenceDomainIsAbsoluteForConstantDataRule)
{
    ASSERT_THROW_MSG(
        DataDescriptorBuilder().setSampleType(SampleType::Int32).setRule(ConstantDataRule()).setReferenceDomainIsAbsolute(False).build(),
        InvalidParameterException,
        "Reference domain is absolute not supported for constant data rule type.");
}

TEST_F(DataDescriptorTest, DisallowReferenceDomainIdWithPostScaling)
{
    ASSERT_THROW_MSG(DataDescriptorBuilder()
                         .setSampleType(SampleType::Float64)
                         .setRule(ExplicitDataRule())
                         .setPostScaling(LinearScaling(2, 3))
                         .setReferenceDomainId("RefDomId")
                         .build(),
                     InvalidParameterException,
                     "Reference domain id not supported with post scaling.");
}

TEST_F(DataDescriptorTest, DisallowReferenceDomainOffsetWithPostScaling)
{
    ASSERT_THROW_MSG(DataDescriptorBuilder()
                         .setSampleType(SampleType::Float64)
                         .setRule(ExplicitDataRule())
                         .setPostScaling(LinearScaling(2, 3))
                         .setReferenceDomainOffset(100)
                         .build(),
                     InvalidParameterException,
                     "Reference domain offset not supported with post scaling.");
}

TEST_F(DataDescriptorTest, DisallowReferenceDomainIsAbsoluteWithPostScaling)
{
    ASSERT_THROW_MSG(DataDescriptorBuilder()
                         .setSampleType(SampleType::Float64)
                         .setRule(ExplicitDataRule())
                         .setPostScaling(LinearScaling(2, 3))
                         .setReferenceDomainIsAbsolute(False)
                         .build(),
                     InvalidParameterException,
                     "Reference domain is absolute not supported with post scaling.");
}

END_NAMESPACE_OPENDAQ
