#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/core_types_utils.h"
#include "opendaq/data_descriptor_ptr.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Definitions

template class VariantConverter<IBaseObject>;

template <>
BaseObjectPtr VariantConverter<IBaseObject>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context);

template <>
ListPtr<IBaseObject> VariantConverter<IBaseObject>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& context);

template <>
OpcUaVariant VariantConverter<IBaseObject>::ToArrayVariant(const ListPtr<IBaseObject>& list,
                                                           const UA_DataType* targetType,
                                                           const ContextPtr& context);
template <>
OpcUaVariant VariantConverter<IBaseObject>::ToVariant(const BaseObjectPtr& object,
                                                      const UA_DataType* targetType,
                                                      const ContextPtr& context);

END_NAMESPACE_OPENDAQ_OPCUA_TMS

#include "opcuatms/converters/selection_converter.h"
#include "opcuatms/converter_maps.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Variant converter

template <>
ListPtr<IBaseObject> VariantConverter<IBaseObject>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (variant.isNull())
        return nullptr;

    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IBaseObject>(variant, context);
    if (variant.isType<UA_Variant>())
        return ListConversionUtils::VariantTypeArrayToList(variant, context);

    const auto list = converters::convertToDaqList(variant, context);
    if (list.assigned())
        return list;

    const auto typeKind = variant.getValue().type->typeKind;
    if (typeKind == UA_DATATYPEKIND_STRUCTURE || typeKind == UA_DATATYPEKIND_OPTSTRUCT)
        return VariantConverter<IStruct>::ToDaqList(variant, context);

    throw ConversionFailedException();
}

template <>
BaseObjectPtr VariantConverter<IBaseObject>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (variant.isNull())
        return nullptr;

    if (!variant.isScalar())
    {
        if (variant.isType<UA_ExtensionObject>())
        {
            const auto extensionObject = ExtensionObject(*static_cast<UA_ExtensionObject*>(variant->data));
            if (extensionObject.isType<UA_DaqKeyValuePair>())
                return VariantConverter<IDict>::ToDaqObject(variant, context);
            if (extensionObject.isType<UA_SelectionEntryStructure>())
                return SelectionVariantConverter::ToDaqObject(variant, context);
        }
        else
        {
            if (variant.isType<UA_DaqKeyValuePair>())
                return VariantConverter<IDict>::ToDaqObject(variant, context);
            if (variant.isType<UA_SelectionEntryStructure>())
                return SelectionVariantConverter::ToDaqObject(variant, context);
        }

        return ToDaqList(variant, context);
    }

    auto decoded = DecodeIfExtensionObject(variant);
    auto unwrapped = UnwrapIfVariant(decoded);

    const auto obj = converters::convertToDaqObject(unwrapped, context);
    if (obj.assigned())
        return obj;

    if (unwrapped.isNull())
        return nullptr; 
    
    const auto typeKind = unwrapped.getValue().type->typeKind;
    if (typeKind == UA_DATATYPEKIND_STRUCTURE || typeKind == UA_DATATYPEKIND_OPTSTRUCT)
        return VariantConverter<IStruct>::ToDaqObject(unwrapped, context);

    throw ConversionFailedException();
}

template <>
OpcUaVariant VariantConverter<IBaseObject>::ToArrayVariant(const ListPtr<IBaseObject>& list,
                                                           const UA_DataType* targetType,
                                                           const ContextPtr& context)
{
    if (targetType == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return ListConversionUtils::ToExtensionObjectArrayVariant<IBaseObject>(list, context);
    if (targetType == &UA_TYPES[UA_TYPES_VARIANT])
        return ListConversionUtils::ToVariantTypeArrayVariant(list, context);

    const auto elementType = list.asPtrOrNull<IListElementType>();
    IntfID elementId;
    elementType->getElementInterfaceId(&elementId);

    if (IntfID::Compare(elementId, IUnknown::Id) && list.getCount() > 0)
        elementId = list[0].asPtr<IInspectable>().getInterfaceIds()[0];

    auto var = converters::convertToArrayVariant(elementId, list, targetType, context);
    if (!var.isNull())
        return var;

    return ListConversionUtils::ToExtensionObjectArrayVariant<IBaseObject>(list, context);
}

template <>
OpcUaVariant VariantConverter<IBaseObject>::ToVariant(const BaseObjectPtr& object,
                                                      const UA_DataType* targetType,
                                                      const ContextPtr& context)
{
    if (!object.assigned())
        return {};
    
    const auto ids = object.asPtr<IInspectable>().getInterfaceIds();
    auto wrapConvertedValue = targetType == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT] || targetType == &UA_TYPES[UA_TYPES_VARIANT];
    wrapConvertedValue = wrapConvertedValue && !object.asPtrOrNull<IList>().assigned();

    if (wrapConvertedValue)
    {
        for (auto id : ids)
        {
            OpcUaVariant converted = converters::convertToVariant(id, object, nullptr, context);
            if (converted.isNull())
                continue;

            OpcUaVariant wrapped;
            if (targetType == &UA_TYPES[UA_TYPES_VARIANT])
            {
                UA_Variant_setScalar(&wrapped.getValue(), converted.newDetachedPointer(), targetType);
            }
            else
            {
                auto extensionObject = ExtensionObject(converted);
                UA_Variant_setScalar(&wrapped.getValue(), extensionObject.newDetachedPointer(), targetType);
            }

            return wrapped;
        }
    }
    else
    {
        for (auto id : ids)
        {
            OpcUaVariant converted = converters::convertToVariant(id, object, targetType, context);
            if (!converted.isNull())
                return converted;
        }
    }

    throw ConversionFailedException();
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
