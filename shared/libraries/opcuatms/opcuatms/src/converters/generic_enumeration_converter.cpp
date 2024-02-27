#include "coretypes/enumeration_factory.h"
#include "coretypes/enumeration_type_factory.h"
#include "coretypes/simple_type_factory.h"
#include "iostream"
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/core_types_utils.h"
#include "opcuatms/extension_object.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <>
EnumerationPtr VariantConverter<IEnumeration>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
{
    std::cout << "DEBUG 2000 : VariantConverter<IEnumeration>::ToDaqObject" << std::endl;

    EnumerationPtr enumeration;
    return enumeration;
}

template <>
OpcUaVariant VariantConverter<IEnumeration>::ToVariant(const EnumerationPtr& object,
                                                       const UA_DataType* targetType,
                                                       const ContextPtr& context)
{
    const auto type = GetUAEnumerationDataTypeByName(object.getEnumerationType().getName());
    std::cout << "DEBUG 199 : object.getEnumerationType().getName(): " << object.getEnumerationType().getName() << std::endl;
    std::cout << "DEBUG 200 : VariantConverter<IEnumeration>::ToVariant typeKind: " << type->typeKind << std::endl;
    if (type == nullptr)
        throw ConversionFailedException{};

    std::cout << "DEBUG 201 : type->memSize: " << type->memSize << std::endl;
    std::cout << "DEBUG 202 : type->typeId.identifier.numeric = " << type->typeId.identifier.numeric << std::endl;

    void* data = UA_new(type); //Is this doing the same as int32?
    OpcUaVariant variant{};
    UA_Variant_setScalar(&variant.getValue(), data, type);

    std::cout << "DEBUG 203 : After Variant creation type->typeId.identifier.numeric =  " << variant.getValue().type->typeId.identifier.numeric << std::endl;
    return variant;
}

template <>
ListPtr<IEnumeration> VariantConverter<IEnumeration>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& context)
{
    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IEnumeration>::ToArrayVariant(const ListPtr<IEnumeration>& list,
                                                            const UA_DataType* /*targetType*/,
                                                            const ContextPtr& context)
{
    throw ConversionFailedException{};
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
