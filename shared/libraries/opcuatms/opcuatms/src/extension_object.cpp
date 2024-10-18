#include <opcuatms/extension_object.h>
#include <opcuatms/converters/variant_converter.h>
#include <opendaq/data_rule_ptr.h>
#include <opcuatms/exceptions.h>
#include <opendaq/range_ptr.h>
#include <opendaq/data_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

ExtensionObject::ExtensionObject()
    : Super()
{
}

ExtensionObject::ExtensionObject(const OpcUaObject<UA_ExtensionObject>& extensionObject)
    : Super(extensionObject)
{
}

ExtensionObject::ExtensionObject(const daq::opcua::OpcUaVariant& variant)
    : Super()
{
    this->setFromVariant(variant);
}

void ExtensionObject::setFromVariant(const daq::opcua::OpcUaVariant& variant)
{
    
    if (variant.isNull())
        UA_ExtensionObject_clear(&value);
    else
        UA_ExtensionObject_setValueCopy(&value, variant->data, variant->type);
}

daq::opcua::OpcUaVariant ExtensionObject::getAsVariant()
{
    if (!isDecoded())
        throw OpcUaObjectNotDecodedException();

    this->markDetached(true);
    auto variant = OpcUaVariant();
    variant->data = this->value.content.decoded.data;
    variant->type = this->value.content.decoded.type;
    return variant;
}

bool ExtensionObject::isDecoded() const
{
    return this->value.encoding == UA_EXTENSIONOBJECT_DECODED || this->value.encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE;
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
