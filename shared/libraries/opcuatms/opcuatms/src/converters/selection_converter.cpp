#include "opcuatms/converters/selection_converter.h"
#include "opcuatms/extension_object.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/struct_converter.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

BaseObjectPtr SelectionVariantConverter::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (!variant.isType<UA_ExtensionObject>())
        throw ConversionFailedException();

    auto data = static_cast<UA_ExtensionObject*>(variant->data);
    auto dict = Dict<IInteger, IBaseObject>();

    OpcUaVariant decodedVariant;
    OpcUaVariant valueVariant;
    bool isDictionary = false;

    for (size_t i = 0; i < variant->arrayLength; i++)
    {
        auto extensionObject = ExtensionObject(data[i]);
        if (extensionObject.isDecoded())
            decodedVariant = extensionObject.getAsVariant();
        else
            throw ConversionFailedException{};

        if (!decodedVariant.isType<UA_SelectionEntryStructure>())
            throw ConversionFailedException{};

        const auto decodedData = static_cast<UA_SelectionEntryStructure*>(decodedVariant->data);

        const auto key = StructConverter<IInteger, UA_Int64>::ToDaqObject(decodedData->key);
        valueVariant.setValue(decodedData->value);
        const auto value = VariantConverter<IBaseObject>::ToDaqObject(valueVariant, context);

        isDictionary = isDictionary || !(static_cast<Int>(i) == key);
        dict.set(key, value);
    }

    if (isDictionary)
        return dict;
    return dict.getValueList();
}

OpcUaVariant SelectionVariantConverter::ToVariant(const BaseObjectPtr& selectionValues, const ContextPtr& context)
{
    if (!selectionValues.assigned())
        throw ConversionFailedException{};

    const auto list = selectionValues.asPtrOrNull<IList>();
    if (list.assigned())
        return ListToVariant(list, context);

    const auto dict = selectionValues.asPtrOrNull<IDict>();
    if (dict.assigned())
        return DictToVariant(dict, context);

    throw ConversionFailedException{};
}

OpcUaObject<UA_SelectionEntryStructure> SelectionVariantConverter::ToKeyValuePair(const IntegerPtr& key,
                                                                                  const BaseObjectPtr& value,
                                                                                  const ContextPtr& context)
{
    OpcUaObject<UA_SelectionEntryStructure> pair;
    pair->key = key;
    pair->value = VariantConverter<IBaseObject>::ToVariant(value, nullptr, context).getDetachedValue();
    return pair;
}

OpcUaVariant SelectionVariantConverter::ListToVariant(const ListPtr<IBaseObject>& selectionValues, const ContextPtr& context)
{
    auto variant = OpcUaVariant();
    if (selectionValues.getCount() == 0)
        return variant;

    const auto type = GetUaDataType<UA_SelectionEntryStructure>();
    const auto arr = static_cast<UA_SelectionEntryStructure*>(UA_Array_new(selectionValues.getCount(), type));
    
    for (SizeT i = 0; i < selectionValues.getCount(); ++i)
    {
        auto tmsStruct = ToKeyValuePair(i, selectionValues[i], context);
        arr[i] = tmsStruct.getDetachedValue();
    }

    UA_Variant_setArray(variant.get(), arr, selectionValues.getCount(), type);
    return variant;
}

OpcUaVariant SelectionVariantConverter::DictToVariant(const DictPtr<IInteger, IBaseObject>& selectionValues, const ContextPtr& context)
{
    auto variant = OpcUaVariant();
    if (selectionValues.getCount() == 0)
        return variant;

    const auto type = GetUaDataType<UA_SelectionEntryStructure>();
    const auto arr = static_cast<UA_SelectionEntryStructure*>(UA_Array_new(selectionValues.getCount(), type));

    int i = 0;
    for (auto [key, value] : selectionValues)
    {
        auto tmsStruct = ToKeyValuePair(key, value, context);
        arr[i] = tmsStruct.getDetachedValue();
        ++i;
    }

    UA_Variant_setArray(variant.get(), arr, selectionValues.getCount(), type);
    return variant;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
