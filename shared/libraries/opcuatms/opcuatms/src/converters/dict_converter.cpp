#include "opcuatms/core_types_utils.h"
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/variant_converter.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

namespace dict_converter
{
    static OpcUaObject<UA_DaqKeyValuePair> ToKeyValuePair(const BaseObjectPtr& key, const BaseObjectPtr& value, const ContextPtr& context)
    {
        OpcUaObject<UA_DaqKeyValuePair> pair;
        pair->key = VariantConverter<IBaseObject>::ToVariant(key, nullptr, context).getDetachedValue();
        pair->value = VariantConverter<IBaseObject>::ToVariant(value, nullptr, context).getDetachedValue();
        return pair;
    }

    static DictPtr<IBaseObject, IBaseObject> ExtensionObjectToDict(const OpcUaVariant& variant, const ContextPtr& context)
    {
        const auto data = static_cast<UA_ExtensionObject*>(variant->data);
        auto dict = Dict<IBaseObject, IBaseObject>();

        OpcUaVariant decodedVariant;
        OpcUaVariant valueVariant;
        OpcUaVariant keyVariant;

        for (size_t i = 0; i < variant->arrayLength; i++)
        {
            auto extensionObject = ExtensionObject(data[i]);
            if (extensionObject.isDecoded())
                decodedVariant = extensionObject.getAsVariant();
            else
                throw ConversionFailedException{};

            if (!decodedVariant.isType<UA_DaqKeyValuePair>())
                throw ConversionFailedException{};

            const auto decodedData = static_cast<UA_DaqKeyValuePair*>(decodedVariant->data);

            keyVariant.setValue(decodedData->key);
            const auto key = VariantConverter<IBaseObject>::ToDaqObject(keyVariant, context);

            valueVariant.setValue(decodedData->value);
            const auto value = VariantConverter<IBaseObject>::ToDaqObject(valueVariant, context);

            dict.set(key, value);
        }
    
        return dict;
    }

    static DictPtr<IBaseObject, IBaseObject> DaqKeyValuePairToDict(const OpcUaVariant& variant, const ContextPtr& context)
    {
        auto dict = Dict<IBaseObject, IBaseObject>();
        OpcUaVariant valueVariant;
        OpcUaVariant keyVariant;
        const auto data = static_cast<UA_DaqKeyValuePair*>(variant->data);

        for (size_t i = 0; i < variant->arrayLength; i++)
        {
            const UA_DaqKeyValuePair keyValuePairData = data[i];

            keyVariant.setValue(keyValuePairData.key);
            const auto key = VariantConverter<IBaseObject>::ToDaqObject(keyVariant, context);

            valueVariant.setValue(keyValuePairData.value);
            const auto value = VariantConverter<IBaseObject>::ToDaqObject(valueVariant, context);

            dict.set(key, value);
        }
        
        return dict;
    }
}

template <>
DictPtr<IBaseObject, IBaseObject> VariantConverter<IDict>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (variant.isScalar())
        throw ConversionFailedException();
    if (variant.isType<UA_ExtensionObject>())
        return dict_converter::ExtensionObjectToDict(variant, context);
    if (variant.isType<UA_DaqKeyValuePair>())
        return dict_converter::DaqKeyValuePairToDict(variant, context);

    throw ConversionFailedException();
}

template <>
OpcUaVariant VariantConverter<IDict>::ToVariant(const DictPtr<IBaseObject, IBaseObject>& object,
                                                const UA_DataType* targetType,
                                                const ContextPtr& context)
{
    if (targetType != nullptr && targetType != &UA_TYPES_TMSBT[UA_TYPES_TMSBT_DAQKEYVALUEPAIR])
        throw ConversionFailedException{};

    auto variant = OpcUaVariant();
    if (object.getCount() == 0)
        return variant;

    const auto type = GetUaDataType<UA_DaqKeyValuePair>();
    const auto arr = static_cast<UA_DaqKeyValuePair*>(UA_Array_new(object.getCount(), type));

    int i = 0;
    for (auto [key, value] : object)
    {
        auto tmsStruct = dict_converter::ToKeyValuePair(key, value, context);
        arr[i] = tmsStruct.getDetachedValue();
        ++i;
    }

    UA_Variant_setArray(variant.get(), arr, object.getCount(), type);
    return variant;
}

template <>
ListPtr<IDict> VariantConverter<IDict>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IDict>::ToArrayVariant(const ListPtr<IDict>& /*list*/,
                                                     const UA_DataType* /*targetType*/,
                                                     const ContextPtr& /*context*/)
{
    throw ConversionFailedException{};
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
