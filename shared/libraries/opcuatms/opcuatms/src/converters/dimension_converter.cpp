#include <opendaq/dimension_factory.h>
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/core_types_utils.h"
#include "opcuatms/extension_object.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IDimension, UA_DimensionDescriptorStructure>;
template class VariantConverter<IDimension>;

// UA_DimensionDescriptorStructure

template <>
DimensionPtr StructConverter<IDimension, UA_DimensionDescriptorStructure>::ToDaqObject(const UA_DimensionDescriptorStructure& tmsStruct,
                                                                                       const ContextPtr& /*context*/)
{
    const auto dimension = DimensionBuilder();

    if (tmsStruct.name)
        dimension->setName(ConvertToDaqCoreString(*tmsStruct.name));

    if (tmsStruct.unit)
        dimension->setUnit(StructConverter<IUnit, UA_EUInformationWithQuantity>::ToDaqObject(*tmsStruct.unit));

    auto ruleObject = ExtensionObject(tmsStruct.dimensionRule);
    if (ruleObject.isDecoded())
        dimension->setRule(VariantConverter<IDimensionRule>::ToDaqObject(ruleObject.getAsVariant()));

    return dimension.build();
}

template <>
OpcUaObject<UA_DimensionDescriptorStructure> StructConverter<IDimension, UA_DimensionDescriptorStructure>::ToTmsType(
    const DimensionPtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_DimensionDescriptorStructure> dimension;

    if (object.getName().assigned())
        dimension->name = ConvertToOpcUaString(object.getName()).newDetachedPointer();

    if (object.getUnit().assigned())
        dimension->unit = StructConverter<IUnit, UA_EUInformationWithQuantity>::ToTmsType(object.getUnit()).newDetachedPointer();

    if (object.getRule().assigned())
    {
        auto ruleObject = ExtensionObject(VariantConverter<IDimensionRule>::ToVariant(object.getRule()));
        dimension->dimensionRule = ruleObject.getDetachedValue();
    }

    return dimension;
}

// Variant converters

template <>
DimensionPtr VariantConverter<IDimension>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (!decodedVariant.isType<UA_DimensionDescriptorStructure>())
        throw ConversionFailedException();

    const auto tmsStruct = static_cast<UA_DimensionDescriptorStructure*>(decodedVariant->data);
    return StructConverter<IDimension, UA_DimensionDescriptorStructure>::ToDaqObject(*tmsStruct);
}

template <>
OpcUaVariant VariantConverter<IDimension>::ToVariant(const DimensionPtr& object,
                                                     const UA_DataType* targetType,
                                                     const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();
    if (targetType == nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_DIMENSIONDESCRIPTORSTRUCTURE])
        variant.setScalar(*StructConverter<IDimension, UA_DimensionDescriptorStructure>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IDimension> VariantConverter<IDimension>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IDimension>(variant);

    return ListConversionUtils::VariantToList<IDimension, UA_DimensionDescriptorStructure>(variant);
}

template <>
OpcUaVariant VariantConverter<IDimension>::ToArrayVariant(const ListPtr<IDimension>& list,
                                                          const UA_DataType* targetType,
                                                          const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_DIMENSIONDESCRIPTORSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDimension, UA_DimensionDescriptorStructure>(list);

    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
