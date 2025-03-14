#include <coretypes/enumeration_factory.h>
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms/core_types_utils.h>
#include <opcuatms/extension_object.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <>
EnumerationPtr VariantConverter<IEnumeration>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (variant.isNull())
        return nullptr;

    if (!context.assigned() || !context.getTypeManager().assigned())
        DAQ_THROW_EXCEPTION(ConversionFailedException, "Generic numeration conversion requires the TypeManager.");

    const auto typeManager = context.getTypeManager();

    // Get UAEnumerationType by name
    const auto DataType = GetUAEnumerationDataTypeByName(variant->type->typeName);

    EnumerationTypePtr Type;

    if (typeManager.hasType(DataType->typeName))
        Type = typeManager.getType(DataType->typeName);
    else
        DAQ_THROW_EXCEPTION(ConversionFailedException, "EnumerationType is not present in Type Manager.");

    DictPtr<IString, IInteger> dictEnumValues = Type.getAsDictionary();

    auto listKeyword = dictEnumValues.getKeyList();
    auto listValues = dictEnumValues.getValueList();
    StringPtr keyword;
    auto data = variant.toInteger();

    for(int i = 0; i < static_cast<int>(listKeyword.getCount()); i++)
    {
        if(listValues[i] == data)
            keyword = listKeyword[i];
    }

    return Enumeration(DataType->typeName, keyword, typeManager);
}

template <>
OpcUaVariant VariantConverter<IEnumeration>::ToVariant(const EnumerationPtr& object,
                                                       const UA_DataType* /*targetType*/,
                                                       const ContextPtr& /*context*/)
{
    const auto dataType = GetUAEnumerationDataTypeByName(object.getEnumerationType().getName());
    assert(dataType->memSize == sizeof(uint32_t));

    const auto intVariant = VariantConverter<IInteger>::ToVariant(object.getIntValue(), &UA_TYPES[UA_TYPES_INT32]);
    OpcUaVariant variant{};
    UA_Variant_setScalarCopy(&variant.getValue(), intVariant->data, dataType);

    return variant;
}

template <>
ListPtr<IEnumeration> VariantConverter<IEnumeration>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& context)
{
    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IEnumeration>::ToArrayVariant(const ListPtr<IEnumeration>& list,
                                                            const UA_DataType* /*targetType*/,
                                                            const ContextPtr& context)
{
    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
