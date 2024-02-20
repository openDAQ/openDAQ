#include "gtest/gtest.h"
#include "opcuatms/converters/variant_converter.h"
#include "coretypes/struct_factory.h"
#include "coretypes/struct_type_factory.h"
#include "coretypes/type_manager_factory.h"
#include "coreobjects/unit_factory.h"
#include "opcuatms/extension_object.h"
#include "opendaq/context_factory.h"
#include "opendaq/data_rule_factory.h"
#include "coretypes/simple_type_factory.h"
#include "opendaq/data_descriptor_factory.h"

using GenericStructConverterTest = testing::Test;

using namespace daq;
using namespace opcua;
using namespace tms;
using namespace opcua;

namespace test_helpers
{
    static ContextPtr setupContext()
    {
        auto typeManager = TypeManager();
        typeManager.addType(StructType("RationalNumber64",
                                       List<IString>("Numerator", "Denominator"),
                                       List<IType>(SimpleType(ctInt), SimpleType(ctInt))));

        typeManager.addType(StructType("DeviceDomainStructure",
                                       List<IString>("Resolution", "TicksSinceOrigin", "Origin", "Unit"),
                                       List<IType>(SimpleType(ctRatio), SimpleType(ctInt), SimpleType(ctString), UnitStructType())));

        typeManager.addType(StructType("DimensionDescriptorStructure",
                                       List<IString>("Name", "DimensionRule", "Unit"),
                                       List<IType>(SimpleType(ctString), SimpleType(ctStruct), UnitStructType())));
        
        typeManager.addType(StructType("ListRuleDescriptionStructure",
                                       List<IString>("Type", "Elements"),
                                       List<IType>(SimpleType(ctString), SimpleType(ctList))));
                
        typeManager.addType(StructType("CustomRuleDescriptionStructure",
                                       List<IString>("Type", "Parameters"),
                                       List<IType>(SimpleType(ctString), SimpleType(ctDict))));
                
        typeManager.addType(StructType("AdditionalParametersType",
                                       List<IString>("Parameters"),
                                       List<IType>(SimpleType(ctList))));

        typeManager.addType(StructType("KeyValuePair",
                                       List<IString>("Key", "Value"),
                                       List<IType>(SimpleType(ctString), SimpleType(ctUndefined))));

        return Context(nullptr, Logger(), typeManager, nullptr);
    }
}

TEST_F(GenericStructConverterTest, TestSimpleStruct)
{
    auto context = test_helpers::setupContext();
    DictPtr<IString, IBaseObject> members = Dict<IString, IBaseObject>();
    members.set("Numerator", 10);
    members.set("Denominator", 50);

    const auto structure = Struct("RationalNumber64", members, context.getTypeManager());

    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);

    const auto rational = static_cast<UA_RationalNumber64*>(var->data);
    ASSERT_EQ(rational->numerator, 10);
    ASSERT_EQ(rational->denominator, 50);

    const auto convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure.getStructType().getName(), convertedStructure.getStructType().getName());
    ASSERT_EQ(structure.getAsDictionary(), convertedStructure.getAsDictionary());
}

TEST_F(GenericStructConverterTest, TestStructWithOtherStructs)
{
    auto context = test_helpers::setupContext();
    DictPtr<IString, IBaseObject> members = Dict<IString, IBaseObject>({{"Resolution", Ratio(10, 20)},
                                                                        {"TicksSinceOrigin", 1000},
                                                                        {"Origin", "origin"},
                                                                        {"Unit", Unit("symbol", -1, "name", "quantity")}});
    const auto structure = Struct("DeviceDomainStructure", members, context.getTypeManager());

    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);
    const auto deviceDomain = static_cast<UA_DeviceDomainStructure*>(var->data);
    ASSERT_EQ(utils::ToStdString(deviceDomain->origin), "origin");
    ASSERT_EQ(utils::ToStdString(deviceDomain->unit.quantity), "quantity");
    ASSERT_EQ(deviceDomain->unit.unitId, -1);
    ASSERT_EQ(deviceDomain->ticksSinceOrigin, 1000);
    ASSERT_EQ(deviceDomain->resolution.numerator, 10);
    
    const auto convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure.getAsDictionary(), convertedStructure.getAsDictionary());
}

TEST_F(GenericStructConverterTest, TestDataDescriptorStruct)
{
    const auto context = test_helpers::setupContext();
    const auto dataDescriptor = DataDescriptorBuilder().build();
    const auto var = VariantConverter<IBaseObject>::ToVariant(dataDescriptor, nullptr, context);
    const StructPtr dataDescriptorStruct = VariantConverter<IBaseObject>::ToDaqObject(var, context);

    ASSERT_EQ(dataDescriptor.asPtr<IStruct>().getStructType(), dataDescriptorStruct.getStructType());
    ASSERT_EQ(dataDescriptor.asPtr<IStruct>().getAsDictionary(), dataDescriptorStruct.getAsDictionary());
}

TEST_F(GenericStructConverterTest, TestStructWithOptionalsAssigned)
{
    auto context = test_helpers::setupContext();
    DictPtr<IString, IBaseObject> members = Dict<IString, IBaseObject>({{"Name", "name"},
                                                                        {"DimensionRule",
                                                                         Struct("LinearRuleDescriptionStructure",
                                                                                Dict<IString, IBaseObject>(
                                                                                {{"Type", "linear"}, {"Start", 10}, {"Delta", 10},
                                                                                 {"Size", 10}}),
                                                                                context.getTypeManager())},
                                                                        {"Unit", Unit("symbol", -1, "name", "quantity")}});
    const auto structure = Struct("DimensionDescriptorStructure", members, context.getTypeManager());

    
    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);

    const auto dimension = static_cast<UA_DimensionDescriptorStructure*>(var->data);
    ASSERT_EQ(utils::ToStdString(*dimension->name), "name");
    ASSERT_EQ(utils::ToStdString(dimension->unit->quantity), "quantity");
    
    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}

TEST_F(GenericStructConverterTest, TestStructWithOptionalsUnassigned1)
{
    auto context = test_helpers::setupContext();
    context.getTypeManager().addType(StructType("LinearRuleDescriptionStructure",
                                                List<IString>("Type", "Start", "Delta", "Size"),
                                                List<IType>(SimpleType(ctString),
                                                            SimpleType(ctInt),
                                                            SimpleType(ctInt),
                                                            SimpleType(ctInt))));
    DictPtr<IString, IBaseObject> members = Dict<IString, IBaseObject>({{"Name", "name"},
                                                                        {"DimensionRule",
                                                                         Struct("LinearRuleDescriptionStructure",
                                                                                Dict<IString, IBaseObject>(
                                                                                {{"Type", "linear"}, {"Start", 10}, {"Delta", 10},
                                                                                 {"Size", nullptr}}),
                                                                                context.getTypeManager())},
                                                                        {"Unit", nullptr}});
    const auto structure = Struct("DimensionDescriptorStructure", members, context.getTypeManager());

    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);

    const auto dimension = static_cast<UA_DimensionDescriptorStructure*>(var->data);
    ASSERT_EQ(utils::ToStdString(*dimension->name), "name");
    ASSERT_EQ(dimension->unit, nullptr);
    
    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}

TEST_F(GenericStructConverterTest, TestStructWithOptionalsUnassigned2)
{
    auto context = test_helpers::setupContext();
    DictPtr<IString, IBaseObject> members = Dict<IString, IBaseObject>({{"Name", "name"},
                                                                        {"DimensionRule",
                                                                         Struct("LinearRuleDescriptionStructure",
                                                                                Dict<IString, IBaseObject>(
                                                                                {{"Type", "linear"}, {"Start", 10}, {"Delta", 10},
                                                                                 {"Size", nullptr}}),
                                                                                context.getTypeManager())},
                                                                        {"Unit", nullptr}});
    const auto structure = Struct("DimensionDescriptorStructure", members, context.getTypeManager());

    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);

    const auto dimension = static_cast<UA_DimensionDescriptorStructure*>(var->data);
    ASSERT_EQ(utils::ToStdString(*dimension->name), "name");
    ASSERT_EQ(dimension->unit, nullptr);
    
    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}

TEST_F(GenericStructConverterTest, TestStructWithArrays1)
{
    auto context = test_helpers::setupContext();
    const auto structure = Struct("ListRuleDescriptionStructure",
                                  Dict<IString, IBaseObject>({{"Type", "list"}, {"Elements", List<IString>("foo", "bar")}}),
                                  context.getTypeManager());

    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);

    const auto rule = static_cast<UA_ListRuleDescriptionStructure*>(var->data);
    ASSERT_EQ(utils::ToStdString(rule->type), "list");
    ASSERT_EQ(rule->elementsSize, 2u);
    
    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}

TEST_F(GenericStructConverterTest, TestStructWithArrays2)
{
    auto context = test_helpers::setupContext();
    const auto structure = Struct("CustomRuleDescriptionStructure",
                                  Dict<IString, IBaseObject>({{"Type", "list"}, {"Parameters", Dict<IString, IBaseObject>({{"foo", "bar"}, {"foo1", "bar1"}})}}),
                                  context.getTypeManager());

    
    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);

    const auto rule = static_cast<UA_CustomRuleDescriptionStructure*>(var->data);
    ASSERT_EQ(utils::ToStdString(rule->type), "list");
    ASSERT_EQ(rule->parametersSize, 2u);

    auto keyVariant = OpcUaVariant(rule->parameters[0].key);
    ASSERT_EQ(VariantConverter<IBaseObject>::ToDaqObject(keyVariant, context), "foo");

    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}

TEST_F(GenericStructConverterTest, TestStructWithArrays3)
{
    auto context = test_helpers::setupContext();
    const auto keyValuePairList = List<IStruct>(
        Struct("KeyValuePair", Dict<IString, IBaseObject>({{"Key", "key1"}, {"Value", "value1"}}), context.getTypeManager()),
        Struct("KeyValuePair", Dict<IString, IBaseObject>({{"Key", "key1"}, {"Value", "value1"}}), context.getTypeManager()));

    const auto structure = Struct("AdditionalParametersType",
                                  Dict<IString, IBaseObject>({{"Parameters", keyValuePairList}}),
                                  context.getTypeManager());
    
    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);
    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}

TEST_F(GenericStructConverterTest, TestStructWithArraysEmptyList)
{
    auto context = test_helpers::setupContext();
    const auto keyValuePairList = List<IStruct>();

    const auto structure = Struct("AdditionalParametersType",
                                  Dict<IString, IBaseObject>({{"Parameters", keyValuePairList}}),
                                  context.getTypeManager());
    
    auto var = VariantConverter<IStruct>::ToVariant(structure, nullptr, context);
    const StructPtr convertedStructure = VariantConverter<IStruct>::ToDaqObject(var, context);
    ASSERT_EQ(structure, convertedStructure);
}
