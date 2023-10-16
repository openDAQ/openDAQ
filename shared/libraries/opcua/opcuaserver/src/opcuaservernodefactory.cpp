#include <opcuaserver/opcuaservernodefactory.h>
#include <opcuaserver/opcuaserver.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaServerNodeFactory::OpcUaServerNodeFactory(OpcUaServer& server)
    : server(server)
{
}

std::unique_ptr<OpcUaServerNode> OpcUaServerNodeFactory::createServerNode(const OpcUaNodeId& nodeId)
{
    return createServerNode(nodeId, server.readNodeClass(nodeId));
}

std::unique_ptr<OpcUaServerNode> OpcUaServerNodeFactory::createServerNode(const OpcUaNodeId& nodeId, OpcUaNodeClass nodeClass)
{
    switch (nodeClass)
    {
        case OpcUaNodeClass::Object:
            return std::make_unique<OpcUaServerObjectNode>(server, nodeId);
        case OpcUaNodeClass::Variable:
            return std::make_unique<OpcUaServerVariableNode>(server, nodeId);
        case OpcUaNodeClass::Method:
            return std::make_unique<OpcUaServerMethodNode>(server, nodeId);
        case OpcUaNodeClass::ObjectType:
            return std::make_unique<OpcUaServerObjectTypeNode>(server, nodeId);
        case OpcUaNodeClass::VariableType:
            return std::make_unique<OpcUaServerVariableTypeNode>(server, nodeId);
        case OpcUaNodeClass::DataType:
            return std::make_unique<OpcUaServerDataTypeNode>(server, nodeId);
        default:
            return std::make_unique<OpcUaServerNode>(server, nodeId);
    }
}

std::unique_ptr<OpcUaServerNode> OpcUaServerNodeFactory::createServerNode(const OpcUaNodeId& nodeId, UA_NodeClass nodeClass)
{
    return createServerNode(nodeId, static_cast<OpcUaNodeClass>(nodeClass));
}

END_NAMESPACE_OPENDAQ_OPCUA
