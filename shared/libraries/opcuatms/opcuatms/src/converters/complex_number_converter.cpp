#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms/core_types_utils.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IComplexNumber, UA_DoubleComplexNumberType>;
template class StructConverter<IComplexNumber, UA_ComplexNumberType>;
template class VariantConverter<IComplexNumber>;

// UA_DoubleComplexNumberType

template <>
ComplexNumberPtr StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToDaqObject(const UA_DoubleComplexNumberType& tmsStruct,
                                                                                          const ContextPtr& /*context*/)
{
    return ComplexNumber(tmsStruct.real, tmsStruct.imaginary);
}

template <>
OpcUaObject<UA_DoubleComplexNumberType> StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToTmsType(
    const ComplexNumberPtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_DoubleComplexNumberType> complex;
    complex->real = object.getReal();
    complex->imaginary = object.getImaginary();
    return complex;
}

// UA_ComplexNumberType

template <>
ComplexNumberPtr StructConverter<IComplexNumber, UA_ComplexNumberType>::ToDaqObject(const UA_ComplexNumberType& tmsStruct,
                                                                                    const ContextPtr& /*context*/)
{
    return ComplexNumber(tmsStruct.real, tmsStruct.imaginary);
}

template <>
OpcUaObject<UA_ComplexNumberType> StructConverter<IComplexNumber, UA_ComplexNumberType>::ToTmsType(const ComplexNumberPtr& object,
                                                                                                   const ContextPtr& /*context*/)
{
    OpcUaObject<UA_ComplexNumberType> complex;
    complex->real = object.getReal();
    complex->imaginary = object.getImaginary();
    return complex;
}

// Variant converter

template <>
ComplexNumberPtr VariantConverter<IComplexNumber>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_DoubleComplexNumberType>())
        return StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToDaqObject(
            *static_cast<UA_DoubleComplexNumberType*>(decodedVariant->data));
    if (decodedVariant.isType<UA_ComplexNumberType>())
        return StructConverter<IComplexNumber, UA_ComplexNumberType>::ToDaqObject(
            *static_cast<UA_ComplexNumberType*>(decodedVariant->data));

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IComplexNumber>::ToVariant(const ComplexNumberPtr& object,
                                                         const UA_DataType* targetType,
                                                         const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_DOUBLECOMPLEXNUMBERTYPE])
        variant.setScalar(*StructConverter<IComplexNumber, UA_DoubleComplexNumberType>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_COMPLEXNUMBERTYPE])
        variant.setScalar(*StructConverter<IComplexNumber, UA_ComplexNumberType>::ToTmsType(object));
    else
        DAQ_THROW_EXCEPTION(ConversionFailedException);
    return variant;
}

template <>
ListPtr<IComplexNumber> VariantConverter<IComplexNumber>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_DoubleComplexNumberType>())
        return ListConversionUtils::VariantToList<IComplexNumber, UA_DoubleComplexNumberType>(variant);
    if (variant.isType<UA_ComplexNumberType>())
        return ListConversionUtils::VariantToList<IComplexNumber, UA_ComplexNumberType>(variant);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IComplexNumber>::ToArrayVariant(const ListPtr<IComplexNumber>& list,
                                                              const UA_DataType* targetType,
                                                              const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_DOUBLECOMPLEXNUMBERTYPE])
        return ListConversionUtils::ToArrayVariant<IComplexNumber, UA_DoubleComplexNumberType>(list);
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_COMPLEXNUMBERTYPE])
        return ListConversionUtils::ToArrayVariant<IComplexNumber, UA_ComplexNumberType>(list);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
