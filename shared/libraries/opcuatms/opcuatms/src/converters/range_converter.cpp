#include <opendaq/range_factory.h>

#include "opcuatms/core_types_utils.h"
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IRange, UA_Range>;
template class VariantConverter<IRange>;

// UA_Range

template <>
RangePtr StructConverter<IRange, UA_Range>::ToDaqObject(const UA_Range& tmsStruct, const ContextPtr& /*context*/)
{
    return Range(tmsStruct.low, tmsStruct.high);
}

template <>
OpcUaObject<UA_Range> StructConverter<IRange, UA_Range>::ToTmsType(const RangePtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_Range> range;
    range->low = object.getLowValue();
    range->high = object.getHighValue();
    return range;
}

// Variant converter

template <>
RangePtr VariantConverter<IRange>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (!decodedVariant.isType<UA_Range>())
        throw ConversionFailedException();
    
    return StructConverter<IRange, UA_Range>::ToDaqObject(*static_cast<UA_Range*>(decodedVariant->data));
}

template <>
OpcUaVariant VariantConverter<IRange>::ToVariant(const RangePtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_RANGE])
        variant.setScalar(*StructConverter<IRange, UA_Range>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IRange> VariantConverter<IRange>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    return ListConversionUtils::VariantToList<IRange, UA_Range>(variant);
}

template <>
OpcUaVariant VariantConverter<IRange>::ToArrayVariant(const ListPtr<IRange>& list,
                                                      const UA_DataType* targetType,
                                                      const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_RANGE])
        return ListConversionUtils::ToArrayVariant<IRange, UA_Range>(list);

    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
