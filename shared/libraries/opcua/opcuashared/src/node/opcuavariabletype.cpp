#include "opcuashared/node/opcuavariabletype.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaVariableType::OpcUaVariableType(const OpcUaNodeId& typeId)
    : OpcUaType(typeId, OpcUaNodeClass::VariableType)
{
}

OpcUaVariableType::~OpcUaVariableType()
{
}

END_NAMESPACE_OPENDAQ_OPCUA
