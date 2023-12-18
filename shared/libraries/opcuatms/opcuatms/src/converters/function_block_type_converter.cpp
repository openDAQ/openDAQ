#include <opcuatms/core_types_utils.h>
#include <opendaq/function_block_type_factory.h>
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/types_daqbsp_generated.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IFunctionBlockType, UA_FunctionBlockInfoStructure>;
template class VariantConverter<IFunctionBlockType>;

// UA_FunctionBlockTypeStructure

template <>
FunctionBlockTypePtr StructConverter<IFunctionBlockType, UA_FunctionBlockInfoStructure>::ToDaqObject(
    const UA_FunctionBlockInfoStructure& tmsStruct, const ContextPtr& /*context*/)
{
    const StringPtr id(ConvertToDaqCoreString(tmsStruct.id));
    const StringPtr name(ConvertToDaqCoreString(tmsStruct.name));
    const StringPtr description(ConvertToDaqCoreString(tmsStruct.description));

    return FunctionBlockType(id, name, description);
}

template <>
OpcUaObject<UA_FunctionBlockInfoStructure> StructConverter<IFunctionBlockType, UA_FunctionBlockInfoStructure>::ToTmsType(
    const FunctionBlockTypePtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_FunctionBlockInfoStructure> type;

    type->id = ConvertToOpcUaString(object.getId()).getDetachedValue();
    type->name = ConvertToOpcUaString(object.getName()).getDetachedValue();
    type->description = ConvertToOpcUaString(object.getDescription()).getDetachedValue();

    return type;
}

// Variant converter

template <>
FunctionBlockTypePtr VariantConverter<IFunctionBlockType>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (!decodedVariant.isType<UA_FunctionBlockInfoStructure>())
        throw ConversionFailedException();

    const auto tmsStruct = static_cast<UA_FunctionBlockInfoStructure*>(decodedVariant->data);
    return StructConverter<IFunctionBlockType, UA_FunctionBlockInfoStructure>::ToDaqObject(*tmsStruct);
}

template <>
OpcUaVariant VariantConverter<IFunctionBlockType>::ToVariant(const FunctionBlockTypePtr& object,
                                                             const UA_DataType* targetType,
                                                             const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_FUNCTIONBLOCKINFOSTRUCTURE])
        variant.setScalar(*StructConverter<IFunctionBlockType, UA_FunctionBlockInfoStructure>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IFunctionBlockType> VariantConverter<IFunctionBlockType>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IFunctionBlockType>(variant);

    return ListConversionUtils::VariantToList<IFunctionBlockType, UA_FunctionBlockInfoStructure>(variant);
}

template <>
OpcUaVariant VariantConverter<IFunctionBlockType>::ToArrayVariant(const ListPtr<IFunctionBlockType>& list,
                                                                  const UA_DataType* targetType,
                                                                  const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_FUNCTIONBLOCKINFOSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IFunctionBlockType, UA_FunctionBlockInfoStructure>(list);

    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
