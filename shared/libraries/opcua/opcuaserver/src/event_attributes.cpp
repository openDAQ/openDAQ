#include <opcuaserver/event_attributes.h>
#include <opcuashared/opcuavariant.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

EventAttributes::EventAttributes()
{
}

void EventAttributes::setTime(UA_UtcTime time)
{
    OpcUaVariant variant;
    variant.setScalar<UA_UtcTime, UtcTimeTypeToUaDataType>(time);
    setAttribute("Time", variant);
}

void EventAttributes::setSeverity(UA_UInt16 eventSeverity)
{
    OpcUaVariant variant;
    variant.setScalar<UA_UInt16>(eventSeverity);
    setAttribute("Severity", variant);
}

void EventAttributes::setMessage(const std::string& message)
{
    setMessage("", message.c_str());
}

void EventAttributes::setMessage(const char* locale, const char* text)
{
    OpcUaObject<UA_LocalizedText> message = UA_LOCALIZEDTEXT_ALLOC(locale, text);
    setMessage(message);
}

void EventAttributes::setMessage(const OpcUaObject<UA_LocalizedText>& message)
{
    OpcUaVariant variant;
    variant.setScalar<UA_LocalizedText>(*message);
    setAttribute("Message", variant);
}

void EventAttributes::setSourceName(const std::string& eventSource)
{
    OpcUaVariant variant(eventSource.c_str());
    setAttribute("SourceName", variant);
}

void EventAttributes::setAttribute(const std::string& attribute, const OpcUaObject<UA_Variant>& value)
{
    setAttribute(UA_QUALIFIEDNAME_ALLOC(0, attribute.c_str()), value);
}

void EventAttributes::setAttribute(const OpcUaObject<UA_QualifiedName>& attribute, const OpcUaObject<UA_Variant>& value)
{
    attributes[attribute] = value;
}

const EventAttributes::AttributesType& EventAttributes::getAttributes() const
{
    return attributes;
}

END_NAMESPACE_OPENDAQ_OPCUA
