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

    if (variant.isNull())
        return nullptr;

    if (!context.assigned() || !context.getTypeManager().assigned())
        throw ConversionFailedException{"Generic numeration conversion requires the TypeManager."};

    const auto typeManager = context.getTypeManager();

    //const auto DataType = variant->type;

    // Get UAEnumerationType by name
    const auto DataType = GetUAEnumerationDataTypeByName(variant->type->typeName);
    std::cout << "DEBUG 2001 : typekind = " << DataType->typeKind << std::endl;
    std::cout << "DEBUG 2002 : typename = " << DataType->typeName << std::endl;
    //std::cout << "DEBUG 2003 : NodeId = "   << DataType->typeId.identifier.numeric << std::endl;

    // Add it to the TypeManager if not present already
    if (!typeManager.hasType(DataType->typeName))
    {
        //Add Enumeration type to Type Manager
        const auto enumExcitationType = EnumerationType("ExcitationTypeEnumeration", List<IString>("DoNotCare", "DCVoltage", "ACVoltage", "ACVoltageRectangle", "ACVoltageSinewave"));
        typeManager.addType(enumExcitationType);
    }
        //typeManager.addType(StructType(DataType->typeName, daqMembers.getKeyList(), memberTypes));

    // Create an Enumeration object and return it
    //return Enumeration("ExcitationTypeEnumeration", "DCVoltage", typeManager);

    EnumerationPtr enumeration;
    return enumeration;
}

template <>
OpcUaVariant VariantConverter<IEnumeration>::ToVariant(const EnumerationPtr& object,
                                                       const UA_DataType* targetType,
                                                       const ContextPtr& context)
{
    const auto DataType = GetUAEnumerationDataTypeByName(object.getEnumerationType().getName());
    std::cout << "DEBUG 199 : object.getEnumerationType().getName(): " << object.getEnumerationType().getName() << std::endl;
    std::cout << "DEBUG 200 : VariantConverter<IEnumeration>::ToVariant typeKind: " << DataType->typeKind << std::endl;
    if (DataType == nullptr)
        throw ConversionFailedException{};

    std::cout << "DEBUG 201 : DataType->memSize: " << DataType->memSize << std::endl;
    std::cout << "DEBUG 202 : DataType->typeId.identifier.numeric = " << DataType->typeId.identifier.numeric << std::endl;

    void* data = UA_new(DataType); //Is this doing the same as int32?

    OpcUaVariant variant{};
    UA_Variant_setScalar(&variant.getValue(), data, DataType);

    std::cout << "DEBUG 204 : After Variant creation type->typeId.identifier.numeric =  " << variant.getValue().type->typeId.identifier.numeric << std::endl;
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
