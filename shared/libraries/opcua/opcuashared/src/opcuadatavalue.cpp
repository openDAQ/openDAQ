#include "opcuashared/opcuadatavalue.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaDataValue::OpcUaDataValue(const UA_DataValue* dataValue)
    : dataValue(dataValue)
    , variant(dataValue->value, true)
{
}

OpcUaDataValue::~OpcUaDataValue()
{
}

bool OpcUaDataValue::hasValue() const
{
    return dataValue->hasValue;
}

const OpcUaVariant& OpcUaDataValue::getValue() const
{
    return variant;
}

const UA_StatusCode& OpcUaDataValue::getStatusCode() const
{
    return dataValue->status;
}

bool OpcUaDataValue::isStatusOK() const
{
    return (getStatusCode() == UA_STATUSCODE_GOOD);
}

const UA_DataValue* OpcUaDataValue::getDataValue() const
{
    return dataValue;
}

OpcUaDataValue::operator const UA_DataValue*() const
{
    return getDataValue();
}

END_NAMESPACE_OPENDAQ_OPCUA
