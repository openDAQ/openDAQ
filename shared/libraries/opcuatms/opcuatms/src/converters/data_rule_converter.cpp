#include <opendaq/data_rule_factory.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/types_daqbsp_generated_handling.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms/core_types_utils.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class VariantConverter<IDataRule>;

// UA_LinearRuleDescription

template <>
DataRulePtr StructConverter<IDataRule, UA_LinearRuleDescriptionStructure>::ToDaqObject(const UA_LinearRuleDescriptionStructure& tmsStruct,
                                                                                       const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "linear")
        throw ConversionFailedException();

    const NumberPtr delta = VariantConverter<INumber>::ToDaqObject(OpcUaVariant(tmsStruct.delta));
    const NumberPtr start = VariantConverter<INumber>::ToDaqObject(OpcUaVariant(tmsStruct.start));

    return LinearDataRule(delta, start);
}

template <>
OpcUaObject<UA_LinearRuleDescriptionStructure> StructConverter<IDataRule, UA_LinearRuleDescriptionStructure>::ToTmsType(
    const DataRulePtr& object, const ContextPtr& /*context*/)
{
    const NumberPtr delta = object.getParameters().get("delta");
    const NumberPtr start = object.getParameters().get("start");

    OpcUaObject<UA_LinearRuleDescriptionStructure> uaRuleDescription;

    uaRuleDescription->type = UA_STRING_ALLOC("linear");
    uaRuleDescription->delta = VariantConverter<INumber>::ToVariant(delta).getDetachedValue();
    uaRuleDescription->start = VariantConverter<INumber>::ToVariant(start).getDetachedValue();

    return uaRuleDescription;
}

// UA_ConstantRuleDescription

template <>
DataRulePtr StructConverter<IDataRule, UA_ConstantRuleDescriptionStructure>::ToDaqObject(
    const UA_ConstantRuleDescriptionStructure& tmsStruct, const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "constant")
        throw ConversionFailedException();

    const NumberPtr value = VariantConverter<INumber>::ToDaqObject(tmsStruct.value);
    return ConstantDataRule();
}

template <>
OpcUaObject<UA_ConstantRuleDescriptionStructure> StructConverter<IDataRule, UA_ConstantRuleDescriptionStructure>::ToTmsType(
    const DataRulePtr& object, const ContextPtr& /*context*/)
{
    const NumberPtr value = Integer(0); // TODO: temporary solution until model is adapted
    OpcUaObject<UA_ConstantRuleDescriptionStructure> uaRuleDescription;

    uaRuleDescription->type = UA_STRING_ALLOC("constant");
    uaRuleDescription->value = VariantConverter<INumber>::ToVariant(value).getDetachedValue();

    return uaRuleDescription;
}

// UA_BaseRuleDescription

template <>
DataRulePtr StructConverter<IDataRule, UA_BaseRuleDescriptionStructure>::ToDaqObject(const UA_BaseRuleDescriptionStructure& tmsStruct,
                                                                                     const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "explicit")
        throw ConversionFailedException();

    return ExplicitDataRule();
}

template <>
OpcUaObject<UA_BaseRuleDescriptionStructure> StructConverter<IDataRule, UA_BaseRuleDescriptionStructure>::ToTmsType(
    const DataRulePtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_BaseRuleDescriptionStructure> uaRuleDescription;
    uaRuleDescription->type = UA_STRING_ALLOC("explicit");
    return uaRuleDescription;
}

// UA_ExplicitDomainRuleDescription

template <>
DataRulePtr StructConverter<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>::ToDaqObject(
    const UA_ExplicitDomainRuleDescriptionStructure& tmsStruct,
    const ContextPtr& /*context*/)
{
    if (tmsStruct.type != "explicit")
        throw ConversionFailedException();

    const NumberPtr minExpectedDelta = VariantConverter<INumber>::ToDaqObject(OpcUaVariant(tmsStruct.minExpectedDelta));
    const NumberPtr maxExpectedDelta = VariantConverter<INumber>::ToDaqObject(OpcUaVariant(tmsStruct.maxExpectedDelta));

    return ExplicitDomainDataRule(minExpectedDelta, maxExpectedDelta);
}

template <>
OpcUaObject<UA_ExplicitDomainRuleDescriptionStructure> StructConverter<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>::ToTmsType(
    const DataRulePtr& object,
    const ContextPtr& /*context*/)
{
    const NumberPtr minExpectedDelta = object.getParameters().get("minExpectedDelta");
    const NumberPtr maxExpectedDelta = object.getParameters().get("maxExpectedDelta");

    OpcUaObject<UA_ExplicitDomainRuleDescriptionStructure> uaRuleDescription;

    uaRuleDescription->type = UA_STRING_ALLOC("explicit");
    uaRuleDescription->minExpectedDelta = VariantConverter<INumber>::ToVariant(minExpectedDelta).getDetachedValue();
    uaRuleDescription->maxExpectedDelta = VariantConverter<INumber>::ToVariant(maxExpectedDelta).getDetachedValue();

    return uaRuleDescription;
}

// UA_CustomRuleDescription

template <>
DataRulePtr StructConverter<IDataRule, UA_CustomRuleDescriptionStructure>::ToDaqObject(const UA_CustomRuleDescriptionStructure& tmsStruct,
                                                                                       const ContextPtr& context)
{
    if (tmsStruct.type != "custom")
        throw ConversionFailedException();

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

    return DataRuleBuilder().setType(DataRuleType::Other).setParameters(params).build();
}

template <>
OpcUaObject<UA_CustomRuleDescriptionStructure> StructConverter<IDataRule, UA_CustomRuleDescriptionStructure>::ToTmsType(
    const DataRulePtr& object, const ContextPtr& context)
{
    OpcUaObject<UA_CustomRuleDescriptionStructure> uaRuleDescription;
    uaRuleDescription->type = UA_STRING_ALLOC("custom");
    auto params = object.getParameters();

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

// Variant converters

template <>
DataRulePtr VariantConverter<IDataRule>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_LinearRuleDescriptionStructure>())
    {
        const auto tmsStruct = static_cast<UA_LinearRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDataRule, UA_LinearRuleDescriptionStructure>::ToDaqObject(*tmsStruct);
    }

    if (decodedVariant.isType<UA_ConstantRuleDescriptionStructure>())
    {
        const auto tmsStruct = static_cast<UA_ConstantRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDataRule, UA_ConstantRuleDescriptionStructure>::ToDaqObject(*tmsStruct);
    }

    if (decodedVariant.isType<UA_CustomRuleDescriptionStructure>())
    {
        const auto tmsStruct = static_cast<UA_CustomRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDataRule, UA_CustomRuleDescriptionStructure>::ToDaqObject(*tmsStruct);
    }
    
    if (decodedVariant.isType<UA_ExplicitDomainRuleDescriptionStructure>())
    {
        const auto tmsStruct = static_cast<UA_ExplicitDomainRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>::ToDaqObject(*tmsStruct);
    }

    if (decodedVariant.isType<UA_BaseRuleDescriptionStructure>())
    {
        const auto tmsStruct = static_cast<UA_BaseRuleDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IDataRule, UA_BaseRuleDescriptionStructure>::ToDaqObject(*tmsStruct);
    }

    throw ConversionFailedException();
}

template <>
OpcUaVariant VariantConverter<IDataRule>::ToVariant(const DataRulePtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();
    
    if (targetType == nullptr)
    {
        switch (object.getType())
        {
            case DataRuleType::Linear:
            {
                variant.setScalar(*StructConverter<IDataRule, UA_LinearRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
            case DataRuleType::Constant:
            {
                variant.setScalar(*StructConverter<IDataRule, UA_ConstantRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
            case DataRuleType::Explicit:
            {
                if (object.getParameters().hasKey("minExpectedDelta") && object.getParameters().hasKey("maxExpectedDelta"))
                    variant.setScalar(*StructConverter<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>::ToTmsType(object));
                else
                    variant.setScalar(*StructConverter<IDataRule, UA_BaseRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
            case DataRuleType::Other:
            {
                variant.setScalar(*StructConverter<IDataRule, UA_CustomRuleDescriptionStructure>::ToTmsType(object));
                break;
            }
        }
    }
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDataRule, UA_LinearRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_CONSTANTRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDataRule, UA_ConstantRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_BASERULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDataRule, UA_BaseRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_CUSTOMRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDataRule, UA_CustomRuleDescriptionStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_EXPLICITDOMAINRULEDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IDataRule> VariantConverter<IDataRule>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IDataRule>(variant);
    if (variant.isType<UA_LinearRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDataRule, UA_LinearRuleDescriptionStructure>(variant);
    if (variant.isType<UA_ConstantRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDataRule, UA_ConstantRuleDescriptionStructure>(variant);
    if (variant.isType<UA_CustomRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDataRule, UA_CustomRuleDescriptionStructure>(variant);
    if (variant.isType<UA_ExplicitDomainRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>(variant);
    if (variant.isType<UA_BaseRuleDescriptionStructure>())
        return ListConversionUtils::VariantToList<IDataRule, UA_BaseRuleDescriptionStructure>(variant);

    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IDataRule>::ToArrayVariant(const ListPtr<IDataRule>& list,
                                                         const UA_DataType* targetType,
                                                         const ContextPtr& /*context*/)
{
    if (targetType == nullptr)
        return ListConversionUtils::ToExtensionObjectArrayVariant<IDataRule>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataRule, UA_LinearRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_CONSTANTRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataRule, UA_ConstantRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_EXPLICITDOMAINRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataRule, UA_ExplicitDomainRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_BASERULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataRule, UA_BaseRuleDescriptionStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_CUSTOMRULEDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataRule, UA_CustomRuleDescriptionStructure>(list);
    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
