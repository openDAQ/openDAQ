#include <opendaq/range_factory.h>

#include "opcuatms/core_types_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IRatio, UA_RationalNumber>;
template class StructConverter<IRatio, UA_RationalNumber64>;
template class VariantConverter<IRatio>;


// UA_RationalNumber

template <>
RatioPtr StructConverter<IRatio, UA_RationalNumber>::ToDaqObject(const UA_RationalNumber& tmsStruct, const ContextPtr& /*context*/)
{
    return Ratio(tmsStruct.numerator, tmsStruct.denominator);
}

template <>
OpcUaObject<UA_RationalNumber> StructConverter<IRatio, UA_RationalNumber>::ToTmsType(const RatioPtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_RationalNumber> ratio;
    ratio->numerator = object.getNumerator();
    ratio->denominator = object.getDenominator();
    return ratio;
}

// UA_RationalNumber64

template <>
RatioPtr StructConverter<IRatio, UA_RationalNumber64>::ToDaqObject(const UA_RationalNumber64& tmsStruct, const ContextPtr& /*context*/)
{
    return Ratio(tmsStruct.numerator, tmsStruct.denominator);
}

template <>
OpcUaObject<UA_RationalNumber64> StructConverter<IRatio, UA_RationalNumber64>::ToTmsType(const RatioPtr& object,
                                                                                         const ContextPtr& /*context*/)
{
    OpcUaObject<UA_RationalNumber64> ratio;
    ratio->numerator = object.getNumerator();
    ratio->denominator = object.getDenominator();
    return ratio;
}

// Variant converter

template <>
RatioPtr VariantConverter<IRatio>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_RationalNumber>())
    {
        const auto tmsStruct = static_cast<UA_RationalNumber*>(decodedVariant->data);
        return StructConverter<IRatio, UA_RationalNumber>::ToDaqObject(*tmsStruct);
    }


    if (decodedVariant.isType<UA_RationalNumber64>())
    {
        const auto tmsStruct = static_cast<UA_RationalNumber64*>(decodedVariant->data);
        return StructConverter<IRatio, UA_RationalNumber64>::ToDaqObject(*tmsStruct);
    }

    throw ConversionFailedException();
}

template <>
OpcUaVariant VariantConverter<IRatio>::ToVariant(const RatioPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES_TMSBT[UA_TYPES_TMSBT_RATIONALNUMBER64])
        variant.setScalar(*StructConverter<IRatio, UA_RationalNumber64>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_RATIONALNUMBER])
        variant.setScalar(*StructConverter<IRatio, UA_RationalNumber>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IRatio> VariantConverter<IRatio>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IRatio>(variant);
    if (variant.isType<UA_RationalNumber>())
        return ListConversionUtils::VariantToList<IRatio, UA_RationalNumber>(variant);
    if (variant.isType<UA_RationalNumber64>())
        return ListConversionUtils::VariantToList<IRatio, UA_RationalNumber64>(variant);

    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IRatio>::ToArrayVariant(const ListPtr<IRatio>& list,
                                                      const UA_DataType* targetType,
                                                      const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES_TMSBT[UA_TYPES_TMSBT_RATIONALNUMBER64])
        return ListConversionUtils::ToArrayVariant<IRatio, UA_RationalNumber64>(list);
    if (targetType == &UA_TYPES[UA_TYPES_RATIONALNUMBER])
        return ListConversionUtils::ToArrayVariant<IRatio, UA_RationalNumber>(list);

    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
