#include "opcuashared/node/opcuanode.h"

#include <utility>
#include "opcuashared/node/opcuatype.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNode::OpcUaNode(const OpcUaNodeId& nodeId, OpcUaNodeClass nodeClass)
    : nodeId(std::move(nodeId))
    , nodeClass(nodeClass)
{
}
OpcUaNode::OpcUaNode(const UA_ReferenceDescription& uaNodeDescription, OpcUaNodeClass nodeClass)
    : nodeId(uaNodeDescription.nodeId.nodeId)
    , browseName(OpcUaNode::GetBrowseName(uaNodeDescription.browseName))
    , displayName(utils::ToStdString(uaNodeDescription.displayName.text))
    , nodeClass(nodeClass)
    , typeId(uaNodeDescription.typeDefinition.nodeId)
{
}

OpcUaNode::~OpcUaNode()
{
}

const OpcUaNodeClass& OpcUaNode::getNodeClass() const
{
    return nodeClass;
}

void OpcUaNode::setNodeClass(const UA_NodeClass& nodeClass)
{
    setNodeClass((OpcUaNodeClass) nodeClass);
}

void OpcUaNode::setNodeClass(OpcUaNodeClass nodeClass)
{
    this->nodeClass = nodeClass;
}

const std::string& OpcUaNode::getBrowseName() const
{
    return browseName;
}

void OpcUaNode::setBrowseName(const std::string& browseName)
{
    this->browseName = browseName;
}

const std::string& OpcUaNode::getDisplayName() const
{
    return displayName;
}

void OpcUaNode::setDisplayName(const std::string& displayName)
{
    this->displayName = displayName;
}

const OpcUaNodeId& OpcUaNode::getNodeId() const
{
    return nodeId;
}

std::string OpcUaNode::GetBrowseName(const UA_QualifiedName& browseName)
{
    return utils::ToStdString(browseName.name);
}

void OpcUaNode::setType(const OpcUaTypePtr& type)
{
    this->typeId = type->getNodeId();
}

void OpcUaNode::setType(const OpcUaNodeId& typeId)
{
    this->typeId = typeId;
}

const OpcUaNodeId& OpcUaNode::getTypeId() const
{
    return typeId;
}

END_NAMESPACE_OPENDAQ_OPCUA
