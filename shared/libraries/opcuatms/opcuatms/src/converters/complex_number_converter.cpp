#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/core_types_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IComplexNumber, UA_DoubleComplexNumberType>;
template class VariantConverter<IComplexNumber>;

// UA_DoubleComplexNumberType

template <>
ComplexNumberPtr StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToDaqObject(const UA_DoubleComplexNumberType& tmsStruct,
                                                                                    const ContextPtr& /*context*/)
{
    return ComplexNumber(tmsStruct.real, tmsStruct.imaginary);
}

template <>
OpcUaObject<UA_DoubleComplexNumberType> StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToTmsType(const ComplexNumberPtr& object,
                                                                                                   const ContextPtr& /*context*/)
{
    OpcUaObject<UA_DoubleComplexNumberType> complex;
    complex->real = object.getReal();
    complex->imaginary = object.getImaginary();
    return complex;
}

// Variant converter

template <>
ComplexNumberPtr VariantConverter<IComplexNumber>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (!decodedVariant.isType<UA_DoubleComplexNumberType>())
        throw ConversionFailedException();

    return StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToDaqObject(*static_cast<UA_DoubleComplexNumberType*>(decodedVariant->data));
}

template <>
OpcUaVariant VariantConverter<IComplexNumber>::ToVariant(const ComplexNumberPtr& object,
                                                         const UA_DataType* targetType,
                                                         const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_COMPLEXNUMBERTYPE])
        variant.setScalar(*StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IComplexNumber> VariantConverter<IComplexNumber>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    return ListConversionUtils::VariantToList<IComplexNumber, UA_DoubleComplexNumberType>(variant);
}

template <>
OpcUaVariant VariantConverter<IComplexNumber>::ToArrayVariant(const ListPtr<IComplexNumber>& list,
                                                              const UA_DataType* targetType,
                                                              const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_COMPLEXNUMBERTYPE])
        return ListConversionUtils::ToArrayVariant<IComplexNumber, UA_DoubleComplexNumberType>(list);

    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
