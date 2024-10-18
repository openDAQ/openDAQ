#include <gtest/gtest.h>
#include <opendaq/range_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/dimension_rule_factory.h>
#include <coretypes/ratio_factory.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuashared/opcuavariant.h>
#include <opcuatms/converters/list_conversion_utils.h>

using VariantListConverterTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;
using namespace opcua;

TEST_F(VariantListConverterTest, Empty)
{
    auto list = List<IRange>();

    const auto variant = VariantConverter<IRange>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IRange>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Number)
{
    auto list = List<INumber>();
    list.pushBack(10);
    list.pushBack(-2);
    list.pushBack(1.5);

    const auto variant = VariantConverter<INumber>::ToArrayVariant(list);
    const auto listOut = VariantConverter<INumber>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Range)
{
    auto list = List<IRange>();
    list.pushBack(Range(1, 10));
    list.pushBack(Range(2, 20));

    const auto variant = VariantConverter<IRange>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IRange>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Ratio)
{
    auto list = List<IRatio>();
    list.pushBack(Ratio(10, 2));
    list.pushBack(Ratio(-2, 5));

    const auto variant = VariantConverter<IRatio>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IRatio>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Unit)
{
    auto list = List<IUnit>();
    list.pushBack(Unit("Symbol", 1, "Name", "q"));
    list.pushBack(Unit("V", 2));

    const auto variant = VariantConverter<IUnit>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IUnit>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Boolean)
{
    auto list = List<IBoolean>();
    list.pushBack(true);
    list.pushBack(false);
    list.pushBack(true);

    const auto variant = VariantConverter<IBoolean>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IBoolean>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Integer)
{
    auto list = List<IInteger>();
    list.pushBack(1000);
    list.pushBack(-15);
    list.pushBack(-22);

    const auto variant = VariantConverter<IInteger>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IInteger>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Float)
{
    auto list = List<IFloat>();
    list.pushBack(1000);
    list.pushBack(3.14);
    list.pushBack(-1.5);

    const auto variant = VariantConverter<IFloat>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IFloat>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, String)
{
    auto list = List<IString>();
    list.pushBack("Hello World!");
    list.pushBack("");
    list.pushBack("hakuna matata");

    const auto variant = VariantConverter<IString>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IString>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, DISABLED_MixedBaseObject)
{
    // disabled: only homogeneous lists are allowed for now

    auto list = List<IBaseObject>();
    list.pushBack(nullptr);
    list.pushBack(true);
    list.pushBack(3);
    list.pushBack(12.5);
    list.pushBack("hakuna matata");

    const auto variant = VariantConverter<IBaseObject>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IBaseObject>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, BaseObject)
{
    auto list = List<IBaseObject>();
    list.pushBack(1.5);
    list.pushBack(2.5);
    list.pushBack(3.5);

    const auto variant = VariantConverter<IBaseObject>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IBaseObject>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, DataDescriptor)
{
    auto id = DataDescriptorBuilder().setSampleType(SampleType::Int32).setName("Id").build();
    auto data = DataDescriptorBuilder().setSampleType(SampleType::UInt8).setName("data").build();
    auto can = DataDescriptorBuilder()
                   .setSampleType(SampleType::Struct)
                   .setName("CAN message")
                   .setStructFields(List<IDataDescriptor>(id, data))
                   .build();

    auto meta = DataDescriptorBuilder().setSampleType(SampleType::Float64).setName("meta").build();

    auto list = List<IDataDescriptor>();
    list.pushBack(can);
    list.pushBack(meta);
    
    const auto variant = VariantConverter<IDataDescriptor>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IDataDescriptor>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, ListDataDescriptor)
{
    auto list = List<IDataDescriptor>();
    list.pushBack(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    ASSERT_NO_THROW(VariantConverter<IDataDescriptor>::ToArrayVariant(list));
    EXPECT_THROW(VariantConverter<IDataDescriptor>::ToDaqList(OpcUaVariant()), ConversionFailedException);
}

TEST_F(VariantListConverterTest, DataDescriptorMetadata)
{
    auto metadata = Dict<IString, IBaseObject>();
    metadata.set("Name", "sine1");
    metadata.set("frequency", "50");
    auto descriptor1 = DataDescriptorBuilder().setSampleType(SampleType::Float64).setName("Sine1").setMetadata(metadata).build();

    auto descriptor2 = DataDescriptorBuilder().setSampleType(SampleType::Float64).setName("Sine2").build();

    auto list = List<IDataDescriptor>();
    list.pushBack(descriptor1);
    list.pushBack(descriptor2);

    const auto variant = VariantConverter<IDataDescriptor, DataDescriptorPtr>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IDataDescriptor, DataDescriptorPtr>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, DataRule)
{
    auto list = List<IDataRule>();
    list.pushBack(LinearDataRule(2.0, 3.0));
    list.pushBack(ConstantDataRule());
    list.pushBack(ExplicitDataRule());

    const auto variant = VariantConverter<IDataRule>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IDataRule>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Dimension)
{
    const auto rule = LinearDimensionRule(2.0, 3.0, 4);
    const auto unit = Unit("V", 1, "voltage", "1");
    auto dx = Dimension(rule, unit, "x");
    auto dy = Dimension(rule, unit, "y");

    auto list = List<IDimension>();
    list.pushBack(dx);
    list.pushBack(dy);

    const auto variant = VariantConverter<IDimension>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IDimension>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, Scaling)
{
    auto list = List<IScaling>();
    list.pushBack(LinearScaling(2.0, 3.0, SampleType::UInt8, ScaledSampleType::Float32));
    list.pushBack(LinearScaling(10.0, 2.0, SampleType::Float64, ScaledSampleType::Float64));

    const auto variant = VariantConverter<IScaling>::ToArrayVariant(list);
    const auto listOut = VariantConverter<IScaling>::ToDaqList(variant);

    Bool eq{false};
    listOut->equals(list, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(VariantListConverterTest, DISABLED_NonExtensionObjectTest)
{
    // disabled: only homogeneous lists are allowed for now

    constexpr size_t listSize = 3;

    auto list = List<IBaseObject>();
    list.pushBack(Integer(10));
    list.pushBack(String("foo"));
    list.pushBack(Floating(123.23));
    
    const auto type = GetUaDataType<UA_Variant>();
    auto arr = (UA_Variant*) UA_Array_new(listSize, type);
    
    for (SizeT i = 0; i < listSize; ++i)
    {
        arr[i] = VariantConverter<IBaseObject>::ToArrayVariant(list).getDetachedValue();
        arr[i].arrayLength = list.getCount();
    }

    UA_Array_delete(arr, 3, type);
}
