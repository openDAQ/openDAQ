#include "opcuatms/core_types_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class VariantConverter<INumber>;

// Variant converter

template <>
NumberPtr VariantConverter<INumber>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isInteger())
        return VariantConverter<IInteger>::ToDaqObject(decodedVariant);

    if (decodedVariant.isDouble())
        return VariantConverter<IFloat>::ToDaqObject(decodedVariant);

    throw ConversionFailedException();
}

template <>
OpcUaVariant VariantConverter<INumber>::ToVariant(const NumberPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    if (targetType == nullptr)
    {
        if (object.getCoreType() == ctInt)
            return VariantConverter<IInteger>::ToVariant(object.getIntValue());

        if (object.getCoreType() == ctFloat)
            return VariantConverter<IFloat>::ToVariant(object.getFloatValue());

        throw ConversionFailedException();
    }

    //TODO: Add if statements for target types of int and float


    throw ConversionFailedException();
}

template <>
ListPtr<INumber> VariantConverter<INumber>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    return ListConversionUtils::ExtensionObjectVariantToList<INumber>(variant);
}

template <>
OpcUaVariant VariantConverter<INumber>::ToArrayVariant(const ListPtr<INumber>& list,
                                                       const UA_DataType* targetType,
                                                       const ContextPtr& /*context*/)
{
    if (targetType == nullptr)
        return ListConversionUtils::ToExtensionObjectArrayVariant<INumber>(list);
    
    //TODO: Add if statements for target types of int and float

    throw ConversionFailedException();
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
