#include "opcuashared/node/opcuanodeobject.h"
#include <open62541/nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNodeObject::OpcUaNodeObject(const OpcUaNodeId& uaNode)
    : OpcUaNode(uaNode, OpcUaNodeClass::Object)
{
}

OpcUaNodeObject::OpcUaNodeObject(const UA_ReferenceDescription& uaNodeDescription)
    : OpcUaNode(uaNodeDescription, OpcUaNodeClass::Object)
{
}

OpcUaNodeObject::~OpcUaNodeObject()
{
}

OpcUaNodeObjectPtr OpcUaNodeObject::instantiateRoot()
{
    return std::make_shared<OpcUaNodeObject>(OPCUANODEID_ROOTFOLDER);
}

OpcUaNodeObjectPtr OpcUaNodeObject::instantiateObjectsFolder()
{
    return std::make_shared<OpcUaNodeObject>(OPCUANODEID_OBJECTSFOLDER);
}

END_NAMESPACE_OPENDAQ_OPCUA
