#include <opcuatms/core_types_utils.h>
#include <coreobjects/unit_factory.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/types_daqbsp_generated.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Template specializations

template class StructConverter<IUnit, UA_EUInformationWithQuantity>;
template class StructConverter<IUnit, UA_EUInformation>;
template class VariantConverter<IUnit>;

// UA_EUInformationWithQuantity

template <>
UnitPtr StructConverter<IUnit, UA_EUInformationWithQuantity>::ToDaqObject(const UA_EUInformationWithQuantity& tmsStruct,
                                                                          const ContextPtr& /*context*/)
{
    const StringPtr displayName(ConvertToDaqCoreString(tmsStruct.displayName.text));
    const StringPtr description(ConvertToDaqCoreString(tmsStruct.description.text));
    const StringPtr quantity(ConvertToDaqCoreString(tmsStruct.quantity));

    return Unit(displayName, tmsStruct.unitId, description, quantity);
}

template <>
OpcUaObject<UA_EUInformationWithQuantity> StructConverter<IUnit, UA_EUInformationWithQuantity>::ToTmsType(const UnitPtr& object,
                                                                                                          const ContextPtr& /*context*/)
{
    OpcUaObject<UA_EUInformationWithQuantity> tmsUnit;

    tmsUnit->namespaceUri = UA_STRING_ALLOC("http://www.opcfoundation.org/UA/units/un/cefact");
    tmsUnit->unitId = object.getId();
    tmsUnit->description = UA_LOCALIZEDTEXT_ALLOC("en-US", object.getName().getCharPtr());
    tmsUnit->displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", object.getSymbol().getCharPtr());
    tmsUnit->quantity = ConvertToOpcUaString(object.getQuantity()).getDetachedValue();

    return tmsUnit;
}

// UA_EUInformation

template <>
UnitPtr StructConverter<IUnit, UA_EUInformation>::ToDaqObject(const UA_EUInformation& tmsStruct, const ContextPtr& /*context*/)
{
    const StringPtr displayName(ConvertToDaqCoreString(tmsStruct.displayName.text));
    const StringPtr description(ConvertToDaqCoreString(tmsStruct.description.text));

    return Unit(displayName, tmsStruct.unitId, description, "");
}

template <>
OpcUaObject<UA_EUInformation> StructConverter<IUnit, UA_EUInformation>::ToTmsType(const UnitPtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_EUInformation> tmsUnit;

    tmsUnit->unitId = object.getId();
    tmsUnit->description = UA_LOCALIZEDTEXT_ALLOC("en-US", object.getName().getCharPtr());
    tmsUnit->displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", object.getSymbol().getCharPtr());

    return tmsUnit;
}

// Variant converter

template <>
UnitPtr VariantConverter<IUnit>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_EUInformationWithQuantity>())
        return StructConverter<IUnit, UA_EUInformationWithQuantity>::ToDaqObject(*static_cast<UA_EUInformationWithQuantity*>(decodedVariant->data));

    if (decodedVariant.isType<UA_EUInformation>())
        return StructConverter<IUnit, UA_EUInformation>::ToDaqObject(*static_cast<UA_EUInformation*>(decodedVariant->data));

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IUnit>::ToVariant(const UnitPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType == &UA_TYPES_DAQBT[UA_TYPES_DAQBT_EUINFORMATIONWITHQUANTITY])
        variant.setScalar(*StructConverter<IUnit, UA_EUInformationWithQuantity>::ToTmsType(object));
    else if (targetType == &UA_TYPES[UA_TYPES_EUINFORMATION])
        variant.setScalar(*StructConverter<IUnit, UA_EUInformation>::ToTmsType(object));
    else
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    return variant;
}

template <>
ListPtr<IUnit> VariantConverter<IUnit>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IUnit>(variant);
    if (variant.isType<UA_EUInformationWithQuantity>())
        return ListConversionUtils::VariantToList<IUnit, UA_EUInformationWithQuantity>(variant);
    if (variant.isType<UA_EUInformation>())
        return ListConversionUtils::VariantToList<IUnit, UA_EUInformation>(variant);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IUnit>::ToArrayVariant(const ListPtr<IUnit>& list,
                                                     const UA_DataType* targetType,
                                                     const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES_DAQBT[UA_TYPES_DAQBT_EUINFORMATIONWITHQUANTITY])
        return ListConversionUtils::ToArrayVariant<IUnit, UA_EUInformationWithQuantity>(list);
    if (targetType == &UA_TYPES[UA_TYPES_EUINFORMATION])
        return ListConversionUtils::ToArrayVariant<IUnit, UA_EUInformation>(list);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
