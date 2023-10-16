#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Bool

template <>
BoolPtr StructConverter<IBoolean, UA_Boolean>::ToDaqObject(const UA_Boolean& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Boolean> StructConverter<IBoolean, UA_Boolean>::ToTmsType(const BoolPtr& object, const ContextPtr& /*context*/)
{
    return OpcUaObject(static_cast<UA_Boolean>(object));
}

template <>
BoolPtr VariantConverter<IBoolean>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isNull())
        return {};
    return variant.toBool();
}

template <>
OpcUaVariant VariantConverter<IBoolean>::ToVariant(const BoolPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_BOOLEAN])
        return OpcUaVariant(static_cast<bool>(object));

    throw ConversionFailedException{};
}

template <>
ListPtr<IBoolean> VariantConverter<IBoolean>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    return ListConversionUtils::VariantToList<IBoolean, UA_Boolean>(variant);
}

template <>
OpcUaVariant VariantConverter<IBoolean>::ToArrayVariant(const ListPtr<IBoolean>& list,
                                                        const UA_DataType* targetType,
                                                        const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_BOOLEAN])
        return ListConversionUtils::ToArrayVariant<IBoolean, UA_Boolean>(list);

    throw ConversionFailedException{};
}

// Int

template <>
IntegerPtr StructConverter<IInteger, UA_Int64>::ToDaqObject(const UA_Int64& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Int64> StructConverter<IInteger, UA_Int64>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<Int>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_UInt64>::ToDaqObject(const UA_UInt64& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_UInt64> StructConverter<IInteger, UA_UInt64>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_UInt64>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_Int32>::ToDaqObject(const UA_Int32& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Int32> StructConverter<IInteger, UA_Int32>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_Int32>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_UInt32>::ToDaqObject(const UA_UInt32& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_UInt32> StructConverter<IInteger, UA_UInt32>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_UInt32>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_UInt16>::ToDaqObject(const UA_UInt16& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_UInt16> StructConverter<IInteger, UA_UInt16>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_UInt16>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_Int16>::ToDaqObject(const UA_Int16& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Int16> StructConverter<IInteger, UA_Int16>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_Int16>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_Byte>::ToDaqObject(const UA_Byte& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Byte> StructConverter<IInteger, UA_Byte>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_Byte>(object)};
}

template <>
IntegerPtr StructConverter<IInteger, UA_SByte>::ToDaqObject(const UA_SByte& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_SByte> StructConverter<IInteger, UA_SByte>::ToTmsType(const IntegerPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_SByte>(object)};
}

template <>
IntegerPtr VariantConverter<IInteger>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isNull())
        return {};
    return variant.toInteger();
}

template <>
OpcUaVariant VariantConverter<IInteger>::ToVariant(const IntegerPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_INT64])
        variant.setScalar(*StructConverter<IInteger, UA_Int64>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_UINT64])
        variant.setScalar(*StructConverter<IInteger, UA_UInt64>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_INT32])
        variant.setScalar(*StructConverter<IInteger, UA_Int32>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_UINT32])
        variant.setScalar(*StructConverter<IInteger, UA_UInt32>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_INT16])
        variant.setScalar(*StructConverter<IInteger, UA_Int16>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_UINT16])
        variant.setScalar(*StructConverter<IInteger, UA_UInt16>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_BYTE])
        variant.setScalar(*StructConverter<IInteger, UA_Byte>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_SBYTE])
        variant.setScalar(*StructConverter<IInteger, UA_SByte>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IInteger> VariantConverter<IInteger>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_Int64>())
        return ListConversionUtils::VariantToList<IInteger, UA_Int64>(variant);
    if (variant.isType<UA_UInt64>())
        return ListConversionUtils::VariantToList<IInteger, UA_UInt64>(variant);
    if (variant.isType<UA_Int32>())
        return ListConversionUtils::VariantToList<IInteger, UA_Int32>(variant);
    if (variant.isType<UA_UInt32>())
        return ListConversionUtils::VariantToList<IInteger, UA_UInt32>(variant);
    if (variant.isType<UA_Int16>())
        return ListConversionUtils::VariantToList<IInteger, UA_Int16>(variant);
    if (variant.isType<UA_UInt16>())
        return ListConversionUtils::VariantToList<IInteger, UA_UInt16>(variant);
    if (variant.isType<UA_Byte>())
        return ListConversionUtils::VariantToList<IInteger, UA_Byte>(variant);
    if (variant.isType<UA_SByte>())
        return ListConversionUtils::VariantToList<IInteger, UA_SByte>(variant);
    
    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IInteger>::ToArrayVariant(const ListPtr<IInteger>& list,
                                                        const UA_DataType* targetType,
                                                        const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_INT64])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_Int64>(list);
    if (targetType == &UA_TYPES[UA_TYPES_UINT64])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_UInt64>(list);
    if (targetType == &UA_TYPES[UA_TYPES_INT32])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_Int32>(list);
    if (targetType == &UA_TYPES[UA_TYPES_UINT32])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_UInt32>(list);
    if (targetType == &UA_TYPES[UA_TYPES_INT16])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_Int16>(list);
    if (targetType == &UA_TYPES[UA_TYPES_UINT16])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_UInt16>(list);
    if (targetType == &UA_TYPES[UA_TYPES_BYTE])
        return ListConversionUtils::ToArrayVariant<IInteger, UA_Byte>(list);
    if (targetType == &UA_TYPES[UA_TYPES_SBYTE])
       return ListConversionUtils::ToArrayVariant<IInteger, UA_SByte>(list);
    
    throw ConversionFailedException{};
}

// Float

template <>
FloatPtr StructConverter<IFloat, UA_Double>::ToDaqObject(const UA_Double& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Double> StructConverter<IFloat, UA_Double>::ToTmsType(const FloatPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_Double>(object)};
}

template <>
FloatPtr StructConverter<IFloat, UA_Float>::ToDaqObject(const UA_Float& value, const ContextPtr& /*context*/)
{
    return value;
}

template <>
OpcUaObject<UA_Float> StructConverter<IFloat, UA_Float>::ToTmsType(const FloatPtr& object, const ContextPtr& /*context*/)
{
    return {static_cast<UA_Float>(object)};
}


template <>
FloatPtr VariantConverter<IFloat>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    return variant.toDouble();
}

template <>
OpcUaVariant VariantConverter<IFloat>::ToVariant(const FloatPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_DOUBLE])
        variant.setScalar(*StructConverter<IFloat, UA_Double>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_FLOAT])
        variant.setScalar(*StructConverter<IFloat, UA_Float>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IFloat> VariantConverter<IFloat>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_Double>())
        return ListConversionUtils::VariantToList<IFloat, UA_Double>(variant);
    if (variant.isType<UA_Float>())
        return ListConversionUtils::VariantToList<IFloat, UA_Float>(variant);

    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IFloat>::ToArrayVariant(const ListPtr<IFloat>& list,
                                                      const UA_DataType* targetType,
                                                      const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_DOUBLE])
        return ListConversionUtils::ToArrayVariant<IFloat, UA_Double>(list);
    if (targetType == &UA_TYPES[UA_TYPES_FLOAT])
        return ListConversionUtils::ToArrayVariant<IFloat, UA_Float>(list);
    
    throw ConversionFailedException{};
}

// String

template <>
StringPtr StructConverter<IString, UA_String>::ToDaqObject(const UA_String& value, const ContextPtr& /*context*/)
{
    return utils::ToStdString(value);
}

template <>
OpcUaObject<UA_String> StructConverter<IString, UA_String>::ToTmsType(const StringPtr& object, const ContextPtr& /*context*/)
{
    return {UA_STRING_ALLOC(object.getCharPtr())};
}

template <>
StringPtr StructConverter<IString, UA_LocalizedText>::ToDaqObject(const UA_LocalizedText& value, const ContextPtr& /*context*/)
{
    return utils::ToStdString(value.text);
}

template <>
OpcUaObject<UA_LocalizedText> StructConverter<IString, UA_LocalizedText>::ToTmsType(const StringPtr& object, const ContextPtr& /*context*/)
{
    return {UA_LOCALIZEDTEXT_ALLOC("", object.getCharPtr())};
}

template <>
StringPtr StructConverter<IString, UA_QualifiedName>::ToDaqObject(const UA_QualifiedName& value, const ContextPtr& /*context*/)
{
    return utils::ToStdString(value.name);
}

template <>
OpcUaObject<UA_QualifiedName> StructConverter<IString, UA_QualifiedName>::ToTmsType(const StringPtr& object, const ContextPtr& /*context*/)
{
    return {UA_QUALIFIEDNAME_ALLOC(0, object.getCharPtr())};
}

template <>
StringPtr VariantConverter<IString>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isNull())
        return {};
    return variant.toString();
}

template <>
OpcUaVariant VariantConverter<IString>::ToVariant(const StringPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_STRING])
        variant.setScalar(*StructConverter<IString, UA_String>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])
        variant.setScalar(*StructConverter<IString, UA_LocalizedText>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_QUALIFIEDNAME])
        variant.setScalar(*StructConverter<IString, UA_QualifiedName>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IString> VariantConverter<IString>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_String>())
        return ListConversionUtils::VariantToList<IString, UA_String>(variant);
    if (variant.isType<UA_LocalizedText>())
        return ListConversionUtils::VariantToList<IString, UA_LocalizedText>(variant);
    if (variant.isType<UA_QualifiedName>())
        return ListConversionUtils::VariantToList<IString, UA_QualifiedName>(variant);

    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IString>::ToArrayVariant(const ListPtr<IString>& list,
                                                       const UA_DataType* targetType,
                                                       const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES[UA_TYPES_STRING])
        return ListConversionUtils::ToArrayVariant<IString, UA_String>(list);
    if (targetType == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])
        return ListConversionUtils::ToArrayVariant<IString, UA_LocalizedText>(list);
    
    throw ConversionFailedException{};
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
