#include <coreobjects/argument_info_factory.h>
#include <opcuatms/core_types_utils.h>
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/types_daqbsp_generated.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IArgumentInfo, UA_Argument>;
template class VariantConverter<IArgumentInfo>;

// UA_Argument

template <>
ArgumentInfoPtr StructConverter<IArgumentInfo, UA_Argument>::ToDaqObject(const UA_Argument& tmsStruct, const ContextPtr& /*context*/)
{
    return ArgumentInfo(ConvertToDaqCoreString(tmsStruct.name), UANodeIdToCoreType(OpcUaNodeId(tmsStruct.dataType)));
}

template <>
OpcUaObject<UA_Argument> StructConverter<IArgumentInfo, UA_Argument>::ToTmsType(const ArgumentInfoPtr& object,
                                                                                const ContextPtr& /*context*/)
{
    if (!object.assigned())
        return {};

    OpcUaObject<UA_Argument> uaArg;
    uaArg->description = UA_LOCALIZEDTEXT_ALLOC("", "");
    uaArg->name = UA_STRING_ALLOC(object.getName().getCharPtr());
    uaArg->dataType = CoreTypeToUANodeID(object.getType()).getDetachedValue();

    // TODO: handle list and dict
    // https://www.open62541.org/doc/0.2/tutorial_server_method.html
    uaArg->valueRank = -1;

    return uaArg;
}

// Variant converter

template <>
ArgumentInfoPtr VariantConverter<IArgumentInfo>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (!decodedVariant.isType<UA_Argument>())
        throw ConversionFailedException();
    
    return StructConverter<IArgumentInfo, UA_Argument>::ToDaqObject(*static_cast<UA_Argument*>(decodedVariant->data));
}

template <>
OpcUaVariant VariantConverter<IArgumentInfo>::ToVariant(const ArgumentInfoPtr& object,
                                                        const UA_DataType* targetType,
                                                        const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_ARGUMENT])
        variant.setScalar(*StructConverter<IArgumentInfo, UA_Argument>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IArgumentInfo> VariantConverter<IArgumentInfo>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IArgumentInfo>(variant);
    return ListConversionUtils::VariantToList<IArgumentInfo, UA_Argument>(variant);
}

template <>
OpcUaVariant VariantConverter<IArgumentInfo>::ToArrayVariant(const ListPtr<IArgumentInfo>& list,
                                                             const UA_DataType* targetType,
                                                             const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_ARGUMENT])
        return ListConversionUtils::ToArrayVariant<IArgumentInfo, UA_Argument>(list);
    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
