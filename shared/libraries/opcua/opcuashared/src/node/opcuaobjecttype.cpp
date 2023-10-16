#include "opcuashared/node/opcuaobjecttype.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaObjectType::OpcUaObjectType(const OpcUaNodeId& typeId)
    : OpcUaType(typeId, OpcUaNodeClass::ObjectType)
{
}

OpcUaObjectType::~OpcUaObjectType()
{
}

END_NAMESPACE_OPENDAQ_OPCUA
