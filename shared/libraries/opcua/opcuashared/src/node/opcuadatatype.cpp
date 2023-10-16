#include "opcuashared/node/opcuadatatype.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaDataType::OpcUaDataType(const UA_ReferenceDescription& uaNodeDescription)
    : OpcUaNode(uaNodeDescription, OpcUaNodeClass::DataType)
{
}

OpcUaDataType::~OpcUaDataType()
{
}

END_NAMESPACE_OPENDAQ_OPCUA
