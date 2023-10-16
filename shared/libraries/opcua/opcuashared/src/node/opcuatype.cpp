#include "opcuashared/node/opcuatype.h"
#include <cassert>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaType::OpcUaType(const OpcUaNodeId& typeId, OpcUaNodeClass nodeClass)
    : OpcUaNode(typeId, nodeClass)
{
    assert(!typeId.isNull());
}

END_NAMESPACE_OPENDAQ_OPCUA
