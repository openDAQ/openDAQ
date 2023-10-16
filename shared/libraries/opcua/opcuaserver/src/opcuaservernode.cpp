#include <opcuaserver/opcuaserver.h>
#include <opcuaserver/opcuaservernode.h>
#include <opcuaserver/opcuaservernodefactory.h>
#include <cassert>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaServerNode::OpcUaServerNode(OpcUaServer& server, const OpcUaNodeId& nodeId)
    : server(server)
    , nodeId(nodeId)
{
    // check if node exists
    server.readNodeClass(nodeId);
}

OpcUaNodeId OpcUaServerNode::getNodeId() const
{
    return nodeId;
}

OpcUaNodeClass OpcUaServerNode::getNodeClass()
{
    return server.readNodeClass(getNodeId());
}

OpcUaObject<UA_QualifiedName> OpcUaServerNode::getBrowseName()
{
    return server.readBrowseName(getNodeId());
}

void OpcUaServerNode::setDisplayName(const std::string& displayName)
{
    OpcUaObject<UA_LocalizedText> localizedText;
    localizedText->text = UA_STRING_ALLOC(displayName.c_str());

    server.setDisplayName(getNodeId(), localizedText);
}

OpcUaObject<UA_LocalizedText> OpcUaServerNode::getDisplayName()
{
    return server.readDisplayName(getNodeId());
}

std::vector<std::unique_ptr<OpcUaServerNode>> OpcUaServerNode::browse(const OpcUaNodeId& referenceTypeId,
                                                                      bool includeSubtypes,
                                                                      OpcUaNodeClass nodeClassMask,
                                                                      UA_BrowseDirection browseDirection)
{
    OpcUaObject<UA_BrowseDescription> browseDescription;
    browseDescription->nodeId = nodeId.copyAndGetDetachedValue();
    browseDescription->browseDirection = browseDirection;
    browseDescription->referenceTypeId = referenceTypeId.copyAndGetDetachedValue();
    browseDescription->nodeClassMask = (UA_UInt32) nodeClassMask;
    browseDescription->includeSubtypes = includeSubtypes;
    browseDescription->resultMask = UA_BROWSERESULTMASK_NODECLASS;

    auto browseResult = server.browse(browseDescription);
    CheckStatusCodeException(browseResult->statusCode, "Browse failed");

    OpcUaServerNodeFactory nodeFactory(server);

    std::vector<std::unique_ptr<OpcUaServerNode>> result;
    for (size_t i = 0; i < browseResult->referencesSize; i++)
    {
        const auto reference = browseResult->references[i];

        OpcUaNodeId nodeId(reference.nodeId.nodeId);
        result.push_back(nodeFactory.createServerNode(nodeId, reference.nodeClass));
    }

    return result;
}

std::vector<std::unique_ptr<OpcUaServerNode>> OpcUaServerNode::browseChildNodes()
{
    return browse(OpcUaNodeId(UA_NS0ID_HIERARCHICALREFERENCES));
}

std::unique_ptr<OpcUaServerNode> OpcUaServerNode::getChildNode(const OpcUaObject<UA_QualifiedName>& qualifiedName)
{
    OpcUaObject<UA_BrowsePathResult> result = UA_Server_browseSimplifiedBrowsePath(server.getUaServer(), *nodeId, 1, qualifiedName.get());
    CheckStatusCodeException(result->statusCode, "Browse failed");
    assert(result->targetsSize == 1);
    OpcUaServerNodeFactory factory(server);
    return factory.createServerNode(result->targets[0].targetId.nodeId);
}

OpcUaServerObjectNode OpcUaServerNode::addObject(AddObjectNodeParams& params)
{
    params.parentNodeId = getNodeId();
    auto newNodeId = server.addObjectNode(params);
    return OpcUaServerObjectNode(server, newNodeId);
}

OpcUaServerObjectNode OpcUaServerNode::addObject(const OpcUaNodeId& nodeId, const std::string& browseName)
{
    AddObjectNodeParams nodeParams(nodeId);
    nodeParams.setBrowseName(browseName);
    return addObject(nodeParams);
}

OpcUaServerVariableNode OpcUaServerNode::addVariable(AddVariableNodeParams& params)
{
    params.parentNodeId = getNodeId();
    auto newNodeId = server.addVariableNode(params);
    return OpcUaServerVariableNode(server, newNodeId);
}

OpcUaServerVariableNode OpcUaServerNode::addVariable(const OpcUaNodeId& nodeId, const std::string& browseName)
{
    AddVariableNodeParams nodeParams(nodeId);
    nodeParams.setBrowseName(browseName);

    return addVariable(nodeParams);
}

void OpcUaServerVariableNode::writeVariantToServer(const OpcUaVariant& var)
{
    server.writeValue(getNodeId(), var);
}

OpcUaVariant OpcUaServerVariableNode::readVariantFromServer()
{
    return server.readValue(getNodeId());
}

void OpcUaServerNode::remove()
{
    server.deleteNode(nodeId);
}

END_NAMESPACE_OPENDAQ_OPCUA
