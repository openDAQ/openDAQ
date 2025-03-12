#include <opendaq/dimension_rule_factory.h>
#include <opcuatms/core_types_utils.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/types_daqbsp_generated_handling.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IDimensionRule, UA_LinearRuleDescriptionStructure>;
template class StructConverter<IDimensionRule, UA_LogRuleDescriptionStructure>;
template class StructConverter<IDimensionRule, UA_ListRuleDescriptionStructure>;
template class VariantConverter<IDimensionRule>;

// UA_LinearRuleDescription

template <>
DimensionRulePtr StructConverter<IDimensionRule, UA_LinearRuleDescriptionStructure>::ToDaqObject(
    const UA_LinearRuleDescriptionStructure& tmsStruct, const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "linear")
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    if (tmsStruct.size == nullptr)
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    const SizeT size = *tmsStruct.size;
    const NumberPtr delta = VariantConverter<INumber>::ToDaqObject(tmsStruct.delta);
    const NumberPtr start = VariantConverter<INumber>::ToDaqObject(tmsStruct.start);
    return LinearDimensionRule(delta, start, size);
}

template <>
OpcUaObject<UA_LinearRuleDescriptionStructure> StructConverter<IDimensionRule, UA_LinearRuleDescriptionStructure>::ToTmsType(
    const DimensionRulePtr& object, const ContextPtr& /*context*/)
{
    const SizeT size = object.getParameters().get("size");
    const NumberPtr delta = object.getParameters().get("delta");
    const NumberPtr start = object.getParameters().get("start");

    OpcUaObject<UA_LinearRuleDescriptionStructure> uaRuleDescription;

    uaRuleDescription->type = UA_STRING_ALLOC("linear");
    uaRuleDescription->delta = VariantConverter<INumber>::ToVariant(delta).getDetachedValue();
    uaRuleDescription->start = VariantConverter<INumber>::ToVariant(start).getDetachedValue();
    uaRuleDescription->size = UA_UInt32_new();
    *uaRuleDescription->size = size;

    return uaRuleDescription;
}

// UA_LogRuleDescriptionStructure

template <>
DimensionRulePtr StructConverter<IDimensionRule, UA_LogRuleDescriptionStructure>::ToDaqObject(
    const UA_LogRuleDescriptionStructure& tmsStruct, const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "log")
        DAQ_THROW_EXCEPTION(ConversionFailedException);
    
    const NumberPtr delta = VariantConverter<INumber>::ToDaqObject(tmsStruct.delta);
    const NumberPtr start = VariantConverter<INumber>::ToDaqObject(tmsStruct.start);
    return LogarithmicDimensionRule(delta, start, tmsStruct.base, tmsStruct.size);
}

template <>
OpcUaObject<UA_LogRuleDescriptionStructure> StructConverter<IDimensionRule, UA_LogRuleDescriptionStructure>::ToTmsType(
    const DimensionRulePtr& object, const ContextPtr& /*context*/)
{
    const SizeT size = object.getParameters().get("size");
    const NumberPtr delta = object.getParameters().get("delta");
    const NumberPtr start = object.getParameters().get("start");
    const Int base = object.getParameters().get("base");

    OpcUaObject<UA_LogRuleDescriptionStructure> uaRuleDescription;

    uaRuleDescription->type = UA_STRING_ALLOC("log");
    uaRuleDescription->size = size;
    uaRuleDescription->delta = VariantConverter<INumber>::ToVariant(delta).getDetachedValue();
    uaRuleDescription->start = VariantConverter<INumber>::ToVariant(start).getDetachedValue();
    uaRuleDescription->base = base;

    return uaRuleDescription;
}

// UA_ListRuleDescription

template <>
DimensionRulePtr StructConverter<IDimensionRule, UA_ListRuleDescriptionStructure>::ToDaqObject(
    const UA_ListRuleDescriptionStructure& tmsStruct, const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "List")
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    const SizeT elementsSize = tmsStruct.elementsSize;
    if (elementsSize == 0)
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    ListPtr<INumber> list = List<INumber>();
    for (SizeT i = 0; i < elementsSize; i++)
    {
        NumberPtr value = VariantConverter<INumber>::ToDaqObject(tmsStruct.elements[i]);
        list.pushBack(value);
    }

    return ListDimensionRule(list);
}

template <>
OpcUaObject<UA_ListRuleDescriptionStructure> StructConverter<IDimensionRule, UA_ListRuleDescriptionStructure>::ToTmsType(
    const DimensionRulePtr& object, const ContextPtr& /*context*/)
{
    ListPtr<INumber> daqList = object.getParameters().get("List");

    OpcUaObject<UA_ListRuleDescriptionStructure> uaRuleDescription;
    uaRuleDescription->elementsSize = daqList.getCount();
    uaRuleDescription->elements = static_cast<UA_Variant*>(UA_Array_new(uaRuleDescription->elementsSize, GetUaDataType<UA_Variant>()));
    uaRuleDescription->type = UA_STRING_ALLOC("List");

    for (SizeT i = 0; i < daqList.getCount(); i++)
        uaRuleDescription->elements[i] = VariantConverter<INumber>::ToVariant(daqList[i]).getDetachedValue();

    return uaRuleDescription;
}

template <>
DimensionRulePtr StructConverter<IDimensionRule, UA_CustomRuleDescriptionStructure>::ToDaqObject(
    const UA_CustomRuleDescriptionStructure& tmsStruct, const ContextPtr& context)
{
    if (tmsStruct.type != "custom")
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    auto params = Dict<IString, IBaseObject>();
    for (size_t i = 0; i < tmsStruct.parametersSize; i++)
    {
        auto value = OpcUaVariant(tmsStruct.parameters[i].value);
        auto key = OpcUaVariant(tmsStruct.parameters[i].key);
        if (key.isString())
        {
            try
            {
                params.set(key.toString(), VariantConverter<IBaseObject>::ToDaqObject(value, context));
            }
            catch (...)
            {
            }
        }
    }
    
    return DimensionRuleBuilder().setType(DimensionRuleType::Other).setParameters(params).build();
}

template <>
OpcUaObject<UA_CustomRuleDescriptionStructure> StructConverter<IDimensionRule, UA_CustomRuleDescriptionStructure>::ToTmsType(
    const DimensionRulePtr& object, const ContextPtr& context)
{
    OpcUaObject<UA_CustomRuleDescriptionStructure> uaRuleDescription;
    uaRuleDescription->type = UA_STRING_ALLOC("custom");
    const auto params = object.getParameters();

    const auto type = GetUaDataType<UA_DaqKeyValuePair>();
    uaRuleDescription->parameters = static_cast<UA_DaqKeyValuePair*>(UA_Array_new(params.getCount(), type));
    uaRuleDescription->parametersSize = params.getCount();
    size_t index = 0;

    for (const auto& [name, value] : params)
    {
        OpcUaObject<UA_DaqKeyValuePair> pair;
        pair->key = VariantConverter<IString>::ToVariant(name).getDetachedValue();
        pair->value = VariantConverter<IBaseObject>::ToVariant(value, nullptr, context).getDetachedValue();
        uaRuleDescription->parameters[index] = pair.getDetachedValue();

        index++;
    }

    return uaRuleDescription;
}

// Variant converter

template <>
DimensionRulePtr VariantConverter<IDimensionRule>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_LinearRuleDescriptionStructure>())
    {
        const auto tmsRuleDescription = static_cast<UA_LinearRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDimensionRule, UA_LinearRuleDescriptionStructure>::ToDaqObject(*tmsRuleDescription);
    }

    if (decodedVariant.isType<UA_LogRuleDescriptionStructure>())
    {
        const auto tmsRuleDescription = static_cast<UA_LogRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDimensionRule, UA_LogRuleDescriptionStructure>::ToDaqObject(*tmsRuleDescription);
    }

    if (decodedVariant.isType<UA_CustomRuleDescriptionStructure>())
    {
        const auto tmsRuleDescription = static_cast<UA_CustomRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDimensionRule, UA_CustomRuleDescriptionStructure>::ToDaqObject(*tmsRuleDescription);
    }

    if (decodedVariant.isType<UA_ListRuleDescriptionStructure>())
    {
        const auto tmsRuleDescription = static_cast<UA_ListRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDimensionRule, UA_ListRuleDescriptionStructure>::ToDaqObject(*tmsRuleDescription);
    }

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IDimensionRule>::ToVariant(const DimensionRulePtr& object,
                                                         const UA_DataType* targetType,
                                                         const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr)
    {
        switch (object.getType())
        {
            case DimensionRuleType::Linear:
            {
                variant.setScalar(*StructConverter<IDimensionRule, UA_LinearRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
            case DimensionRuleType::Logarithmic:
            {
                variant.setScalar(*StructConverter<IDimensionRule, UA_LogRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
            case DimensionRuleType::List:
            {
                variant.setScalar(*StructConverter<IDimensionRule, UA_ListRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
            case DimensionRuleType::Other:
            {
                variant.setScalar(*StructConverter<IDimensionRule, UA_CustomRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
        }
    }
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDimensionRule, UA_LinearRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LOGRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDimensionRule, UA_LogRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LISTRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDimensionRule, UA_ListRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_CUSTOMRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDimensionRule, UA_CustomRuleDescriptionStructure>::ToTmsType(object));
    else
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    return variant;
}

template <>
ListPtr<IDimensionRule> VariantConverter<IDimensionRule>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IDimensionRule>(variant);
    if (variant.isType<UA_LinearRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDimensionRule, UA_LinearRuleDescriptionStructure>(variant);
    if (variant.isType<UA_LogRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDimensionRule, UA_LogRuleDescriptionStructure>(variant);
    if (variant.isType<UA_ListRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDimensionRule, UA_ListRuleDescriptionStructure>(variant);
    if (variant.isType<UA_CustomRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDimensionRule, UA_CustomRuleDescriptionStructure>(variant);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IDimensionRule>::ToArrayVariant(const ListPtr<IDimensionRule>& list,
                                                              const UA_DataType* targetType,
                                                              const ContextPtr& /*context*/)
{
    if (targetType == nullptr)
        return ListConversionUtils::ToExtensionObjectArrayVariant<IDimensionRule>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDimensionRule, UA_LinearRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LOGRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDimensionRule, UA_LogRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LISTRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDimensionRule, UA_ListRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_CUSTOMRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDimensionRule, UA_CustomRuleDescriptionStructure>(list);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
