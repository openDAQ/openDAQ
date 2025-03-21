#include <opcuatms/converters/property_object_conversion_utils.h>
#include <opcuatms/converters/variant_converter.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

OpcUaVariant PropertyObjectConversionUtils::ToDictVariant(const PropertyObjectPtr& obj)
{
    if (!obj.assigned())
    {
        auto variant = OpcUaVariant();
        UA_Variant_setArray(variant.get(), nullptr, 0, &UA_TYPES_DAQBT[UA_TYPES_DAQBT_DAQKEYVALUEPAIR]);
        return variant;
    }

    auto dict = Dict<IString, IBaseObject>();
    const auto properties = obj.getAllProperties();

    for (const auto& prop : properties)
    {
        const auto key = prop.getName();
        const auto val = obj.getPropertyValue(key);
        dict.set(key, val);
    }

    return VariantConverter<IDict>::ToVariant(dict);
}

void PropertyObjectConversionUtils::ToPropertyObject(const OpcUaVariant& variant, PropertyObjectPtr& objOut)
{
    const auto dict = VariantConverter<IDict>::ToDaqObject(variant);

    for (const auto& entry : dict)
    {
        if (!objOut.hasProperty(entry.first))
        {
            auto property = PropertyBuilder(entry.first).setValueType(entry.second.getCoreType())
                                                        .setDefaultValue(entry.second)
                                                        .build();
            objOut.addProperty(property);
        }
        else
        {
            objOut.setPropertyValue(entry.first, entry.second);
        }
    }
}

PropertyObjectPtr PropertyObjectConversionUtils::ClonePropertyObject(const PropertyObjectPtr& obj)
{
    // This is a workaround until PropertyObject implemnts IClonable.

    auto serializer = JsonSerializer();
    auto deserializer = JsonDeserializer();
    obj.serialize(serializer);
    const auto clone = deserializer.deserialize(serializer.getOutput());
    return clone;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
