#include "gtest/gtest.h"
#include <opendaq/range_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/function_block_type_factory.h>
#include "coretypes/ratio_factory.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuashared/opcuavariant.h"

using VariantConverterTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;
using namespace opcua;

static ListPtr<IDimension> CreateTestDimensions()
{
    auto list = List<IDimension>();

    const auto rule1 = LinearDimensionRule(2.0, 3.0, 4);
    const auto unit1 = Unit("V", 1, "voltage", "1");
    auto dimension1 = Dimension(rule1, unit1, "x");
    list.pushBack(dimension1);

    const auto rule2 = LogarithmicDimensionRule(1.0, 4.0, 2, 9);
    auto dimension2 = Dimension(rule2);
    list.pushBack(dimension2);

    return list;
}

static DataDescriptorBuilderPtr CreateTestStructDescriptorBuilder()
{
    // struct Meta {
    //     string description;
    // }
    //
    // struct CanMessage {
    //     int id;
    //     byte data;
    //     Meta meta;
    // };

    auto id = DataDescriptorBuilder().setSampleType(SampleType::Int32).setName("id").build();
    auto data = DataDescriptorBuilder().setSampleType(SampleType::UInt8).setName("data").build();

    auto desc = DataDescriptorBuilder()
                .setSampleType(SampleType::String)
                .setName("description")
                .setUnit(Unit("V", 1, "voltage", "quantity"))
                .setValueRange(Range(1, 10))
                .setRule(ExplicitDataRule())
                .setOrigin("2022-11-30T10:53:06")
                .build();

    auto meta = DataDescriptorBuilder()
                    .setSampleType(SampleType::Struct)
                    .setName("meta")
                    .setDimensions(CreateTestDimensions())
                    .setStructFields(List<IDataDescriptor>(desc))
                    .build();

    auto can = DataDescriptorBuilder()
                   .setSampleType(SampleType::Struct)
                   .setName("CAN message")
                   .setStructFields(List<IDataDescriptor>(id, data, meta));

    return can;
}

static DataDescriptorPtr CreateTestStructDescriptor()
{
    return CreateTestStructDescriptorBuilder().build();
}

TEST_F(VariantConverterTest, Range)
{
    const RangePtr daqRange2 = Range(2, 20);
    const RangePtr daqRange3 = Range(3, 30);

    const auto variant = VariantConverter<IRange>::ToVariant(daqRange2);
    const auto daqRangeOut = VariantConverter<IRange>::ToDaqObject(variant);

    ASSERT_TRUE(daqRangeOut.equals(daqRange2));
    ASSERT_FALSE(daqRangeOut.equals(daqRange3));
}

TEST_F(VariantConverterTest, Unit)
{
    const UnitPtr daqUnit2 = Unit("V", 2, "measured voltage", "voltage");
    const UnitPtr daqUnit3 = Unit("m", 3, "height of defender", "lenght");

    const auto variant = VariantConverter<IUnit>::ToVariant(daqUnit2);
    const auto daqUnitOut = VariantConverter<IUnit>::ToDaqObject(variant);

    ASSERT_TRUE(daqUnitOut.equals(daqUnit2));
    ASSERT_FALSE(daqUnitOut.equals(daqUnit3));
}

TEST_F(VariantConverterTest, UnitMissingFields)
{
    auto daqUnit = UnitBuilder().setSymbol("s").build();
    const auto variant = VariantConverter<IUnit>::ToVariant(daqUnit);
    const auto daqUnitOut = VariantConverter<IUnit>::ToDaqObject(variant);
    ASSERT_TRUE(daqUnitOut.equals(daqUnit));
}

TEST_F(VariantConverterTest, Bool)
{
    auto variant = VariantConverter<IBoolean>::ToVariant(True);
    auto daqBoolOut = VariantConverter<IBoolean>::ToDaqObject(variant);
    ASSERT_EQ(daqBoolOut, True);

    variant = VariantConverter<IBoolean>::ToVariant(true);
    daqBoolOut = VariantConverter<IBoolean>::ToDaqObject(variant);
    ASSERT_EQ(daqBoolOut, true);

    variant = VariantConverter<IBoolean>::ToVariant(False);
    daqBoolOut = VariantConverter<IBoolean>::ToDaqObject(variant);
    ASSERT_EQ(daqBoolOut, False);

    variant = VariantConverter<IBoolean>::ToVariant(false);
    daqBoolOut = VariantConverter<IBoolean>::ToDaqObject(variant);
    ASSERT_EQ(daqBoolOut, false);
}

TEST_F(VariantConverterTest, Int)
{
    auto variant = VariantConverter<IInteger>::ToVariant(55);
    auto daqIntOut = VariantConverter<IInteger>::ToDaqObject(variant);
    ASSERT_EQ(daqIntOut, 55);
}

TEST_F(VariantConverterTest, Float)
{
    auto variant = VariantConverter<IFloat>::ToVariant(55.55);
    auto daqFloatOut = VariantConverter<IFloat>::ToDaqObject(variant);
    ASSERT_EQ(daqFloatOut, 55.55);
}

TEST_F(VariantConverterTest, Number)
{
    const NumberPtr number1 = 10;
    const NumberPtr number2 = 33.3;
    const NumberPtr number3 = -27;

    NumberPtr numberOut;
    OpcUaVariant variant;

    variant = VariantConverter<INumber>::ToVariant(number1);
    numberOut = VariantConverter<INumber>::ToDaqObject(variant);
    ASSERT_TRUE(numberOut.equals(number1));

    variant = VariantConverter<INumber>::ToVariant(number2);
    numberOut = VariantConverter<INumber>::ToDaqObject(variant);
    ASSERT_TRUE(numberOut.equals(number2));

    variant = VariantConverter<INumber>::ToVariant(number3);
    numberOut = VariantConverter<INumber>::ToDaqObject(variant);
    ASSERT_TRUE(numberOut.equals(number3));
    ASSERT_FALSE(numberOut.equals(number2));
}

TEST_F(VariantConverterTest, String)
{
    StringPtr hello = "Hello World!";

    auto variant = VariantConverter<IString>::ToVariant(hello);
    auto stringOut = VariantConverter<IString>::ToDaqObject(variant);
    ASSERT_EQ(stringOut, hello);
}

TEST_F(VariantConverterTest, BaseObject)
{
    BaseObjectPtr object;
    BaseObjectPtr objectOut;
    OpcUaVariant variant;

    object = nullptr;
    variant = VariantConverter<IBaseObject>::ToVariant(object);
    objectOut = VariantConverter<IBaseObject>::ToDaqObject(variant);
    ASSERT_EQ(objectOut, object);

    object = true;
    variant = VariantConverter<IBaseObject>::ToVariant(object);
    objectOut = VariantConverter<IBaseObject>::ToDaqObject(variant);
    ASSERT_EQ(objectOut, object);

    object = -3;
    variant = VariantConverter<IBaseObject>::ToVariant(object);
    objectOut = VariantConverter<IBaseObject>::ToDaqObject(variant);
    ASSERT_EQ(objectOut, object);

    object = 12.5;
    variant = VariantConverter<IBaseObject>::ToVariant(object);
    objectOut = VariantConverter<IBaseObject>::ToDaqObject(variant);
    ASSERT_EQ(objectOut, object);

    object = "Hello World!";
    variant = VariantConverter<IBaseObject>::ToVariant(object);
    objectOut = VariantConverter<IBaseObject>::ToDaqObject(variant);
    ASSERT_EQ(objectOut, object);
}

TEST_F(VariantConverterTest, LinearDataRule)
{
    const DataRulePtr daqDataRule = LinearDataRule(2.0, 3.0);
    const DataRulePtr daqDataRuleWrong = LinearDataRule(1.0, 4.0);

    const auto variant = VariantConverter<IDataRule>::ToVariant(daqDataRule);
    const auto daqDataRuleOut = VariantConverter<IDataRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqDataRuleOut.equals(daqDataRule));
    ASSERT_FALSE(daqDataRuleOut.equals(daqDataRuleWrong));
}

TEST_F(VariantConverterTest, ConstantDataRule)
{
    const DataRulePtr daqDataRule = ConstantDataRule();

    const auto variant = VariantConverter<IDataRule>::ToVariant(daqDataRule);
    const auto daqDataRuleOut = VariantConverter<IDataRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqDataRuleOut.equals(daqDataRule));
}

TEST_F(VariantConverterTest, ExplicitDataRule)
{
    const DataRulePtr daqDataRule = ExplicitDataRule();

    const auto variant = VariantConverter<IDataRule>::ToVariant(daqDataRule);
    const auto daqDataRuleOut = VariantConverter<IDataRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqDataRuleOut.equals(daqDataRule));
}

TEST_F(VariantConverterTest, ExplicitDomainDataRule)
{
    const DataRulePtr daqDataRule = ExplicitDomainDataRule(5.123, 10);

    const auto variant = VariantConverter<IDataRule>::ToVariant(daqDataRule);
    const auto daqDataRuleOut = VariantConverter<IDataRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqDataRuleOut.equals(daqDataRule));
}

TEST_F(VariantConverterTest, CustomDataRule)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("count", 1);
    params.set("type", "apple");
    params.set("type1", "gala");
    params.set("type2", "fuji");
    params.set("weight", 1.123);

    auto daqDataRule = DataRuleBuilder().setType(DataRuleType::Other).setParameters(params).build();

    const auto variant = VariantConverter<IDataRule>::ToVariant(daqDataRule);
    const auto daqDataRuleOut = VariantConverter<IDataRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqDataRuleOut.equals(daqDataRule));
}

TEST_F(VariantConverterTest, DataDescriptor)
{
    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Float64)
                          .setName("Value 1")
                          .setUnit(Unit("V", 1, "voltage", "quantity"))
                          .setValueRange(Range(1, 10))
                          .setRule(ExplicitDataRule())
                          .setDimensions(CreateTestDimensions())
                          .setPostScaling(LinearScaling(10, 2))
                          .build();

    auto variant = VariantConverter<IDataDescriptor>::ToVariant(descriptor);
    auto descriptorOut = VariantConverter<IDataDescriptor>::ToDaqObject(variant);

    ASSERT_TRUE(descriptorOut.equals(descriptor));
}

TEST_F(VariantConverterTest, DataDescriptorEmpty)
{
    auto daqDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    auto variant = VariantConverter<IDataDescriptor>::ToVariant(daqDescriptor);
    auto descriptorOut = VariantConverter<IDataDescriptor>::ToDaqObject(variant);
    ASSERT_TRUE(descriptorOut.equals(daqDescriptor));
}

TEST_F(VariantConverterTest, StructDataDescriptor)
{
    auto descriptor = CreateTestStructDescriptor();
    auto descriptorWrong = DataDescriptorBuilder().setSampleType(SampleType::Float64).setName("Descriptor wrong").build();

    auto variant = VariantConverter<IDataDescriptor>::ToVariant(descriptor);
    auto descriptorOut = VariantConverter<IDataDescriptor>::ToDaqObject(variant);

    ASSERT_TRUE(descriptorOut.equals(descriptor));
    ASSERT_FALSE(descriptorOut.equals(descriptorWrong));
}

TEST_F(VariantConverterTest, StructDescriptorEmpty)
{
    auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    auto variant = VariantConverter<IDataDescriptor>::ToVariant(descriptor);
    auto descriptorOut = VariantConverter<IDataDescriptor>::ToDaqObject(variant);
    ASSERT_TRUE(descriptorOut.equals(descriptor));
}

TEST_F(VariantConverterTest, DataDescriptorMetadata)
{
    auto metadata = Dict<IString, IBaseObject>();
    metadata.set("name", "sine1");
    metadata.set("frequency", "50");

    auto descriptor = CreateTestStructDescriptorBuilder().setName("Sine1").setMetadata(metadata).build();
    auto descriptorWrong = DataDescriptorBuilder().setSampleType(SampleType::Float64).setName("Wrong descriptor").build();

    auto variant = VariantConverter<IDataDescriptor, DataDescriptorPtr>::ToVariant(descriptor);
    auto descriptorOut = VariantConverter<IDataDescriptor, DataDescriptorPtr>::ToDaqObject(variant);

    ASSERT_TRUE(descriptorOut.equals(descriptor));
    ASSERT_FALSE(descriptorOut.equals(descriptorWrong));
}

TEST_F(VariantConverterTest, LinearDimensionRule)
{
    const DimensionRulePtr daqRule = LinearDimensionRule(2.0, 3.0, 4);
    const DimensionRulePtr daqRuleWrong = LinearDimensionRule(1.0, 4.0, 9);

    const auto variant = VariantConverter<IDimensionRule>::ToVariant(daqRule);
    const auto daqRuleOut = VariantConverter<IDimensionRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqRuleOut.equals(daqRule));
    ASSERT_FALSE(daqRuleOut.equals(daqRuleWrong));
}

TEST_F(VariantConverterTest, LogDimensionRule)
{
    const DimensionRulePtr daqRule = LogarithmicDimensionRule(2.0, 3.0, 10, 4);
    const DimensionRulePtr daqRuleWrong = LogarithmicDimensionRule(1.0, 4.0, 2, 9);

    const auto variant = VariantConverter<IDimensionRule>::ToVariant(daqRule);
    const auto daqRuleOut = VariantConverter<IDimensionRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqRuleOut.equals(daqRule));
    ASSERT_FALSE(daqRuleOut.equals(daqRuleWrong));
}

TEST_F(VariantConverterTest, ListDimensionRule)
{
    ListPtr<INumber> list{10, 20, 30};
    ListPtr<INumber> listWrong{1, 2, 3};
    const DimensionRulePtr daqRule = ListDimensionRule(list);
    const DimensionRulePtr daqRuleWrong = ListDimensionRule(listWrong);

    const auto variant = VariantConverter<IDimensionRule>::ToVariant(daqRule);
    const auto daqRuleOut = VariantConverter<IDimensionRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqRuleOut.equals(daqRule));
    ASSERT_FALSE(daqRuleOut.equals(daqRuleWrong));
}

TEST_F(VariantConverterTest, CustomDimensionRule)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("count", 1);
    params.set("type", "apple");
    params.set("type1", "gala");
    params.set("type2", "fuji");
    params.set("weight", 1.123);

    auto daqDimensionRule = DimensionRuleBuilder().setType(DimensionRuleType::Other).setParameters(params).build();

    const auto variant = VariantConverter<IDimensionRule>::ToVariant(daqDimensionRule);
    const auto daqDimensionRuleOut = VariantConverter<IDimensionRule>::ToDaqObject(variant);

    ASSERT_TRUE(daqDimensionRuleOut.equals(daqDimensionRule));
}

TEST_F(VariantConverterTest, LinearScaling)
{
    const ScalingPtr daqScaling = LinearScaling(2.0, 3.0, SampleType::UInt8, ScaledSampleType::Float32);
    const ScalingPtr daqScalingWrong = LinearScaling(1.0, 4.0, SampleType::Int16, ScaledSampleType::Float64);

    const auto variant = VariantConverter<IScaling>::ToVariant(daqScaling);
    auto daqScalingOut = VariantConverter<IScaling>::ToDaqObject(variant);

    ASSERT_TRUE(daqScalingOut.equals(daqScaling));
    ASSERT_FALSE(daqScalingOut.equals(daqScalingWrong));
}

TEST_F(VariantConverterTest, Dimension)
{
    const auto rule = LinearDimensionRule(2.0, 3.0, 4);
    const auto unit = Unit("V", 1, "voltage", "1");
    auto dimension = Dimension(rule, unit, "x");

    auto dimensionWrong = Dimension(rule, unit, "wrong");

    const auto variant = VariantConverter<IDimension>::ToVariant(dimension);
    auto dimensionOut = VariantConverter<IDimension>::ToDaqObject(variant);

    ASSERT_TRUE(dimensionOut.equals(dimension));
    ASSERT_FALSE(dimensionOut.equals(dimensionWrong));
}

TEST_F(VariantConverterTest, Ratio)
{
    auto ratio = Ratio(1, 2);
    auto ratioWrong = Ratio(1, 10);

    const auto variant = VariantConverter<IRatio>::ToVariant(ratio);
    auto ratioOut = VariantConverter<IRatio>::ToDaqObject(variant);

    ASSERT_TRUE(ratioOut.equals(ratio));
    ASSERT_FALSE(ratioOut.equals(ratioWrong));
}

TEST_F(VariantConverterTest, FunctionBlockType)
{
    const FunctionBlockTypePtr fbType = FunctionBlockType("UNIQUE ID", "NAME", "DESCRIPTION");

    const auto variant = VariantConverter<IFunctionBlockType>::ToVariant(fbType);
    const auto fbTypeOut = VariantConverter<IFunctionBlockType>::ToDaqObject(variant);

    ASSERT_EQ(fbTypeOut.getId(), fbType.getId());
    ASSERT_EQ(fbTypeOut.getName(), fbType.getName());
    ASSERT_EQ(fbTypeOut.getDescription(), fbType.getDescription());
}
