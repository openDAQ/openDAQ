#include "opcuaclient/browser/opcuanodevisitor.h"
#include <opcuashared/opcuaobject.h>
#include <opcuaclient/browse_request.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNodeVisitor::OpcUaNodeVisitor(const OpcUaClientPtr& client, OpcUaNodeClass nodeClassMask, bool recursive)
    : client(client)
    , browseMask(nodeClassMask)
    , recursive(recursive)
{
}

OpcUaNodeVisitor::~OpcUaNodeVisitor()
{
}

OpcUaNodeId OpcUaNodeVisitor::getOpcUaNodeId(const UA_ReferenceDescription& referenceDescription)
{
    return OpcUaNodeId(referenceDescription.nodeId.nodeId);
}

const OpcUaClientPtr& OpcUaNodeVisitor::getClient()
{
    return client;
}

void OpcUaNodeVisitor::setRequestedMaxReferencesPerNode(uint32_t requestedMaxReferencesPerNode)
{
    this->requestedMaxReferencesPerNode = requestedMaxReferencesPerNode;
}

void OpcUaNodeVisitor::traverse(const UA_ReferenceDescription& reference)
{
    if (recursive)
        traverse(getOpcUaNodeId(reference));
}

void OpcUaNodeVisitor::traverse(const OpcUaNodeId& startNodeId)
{
    BrowseRequest request(startNodeId, browseMask);
    OpcUaBrowser browser(request, client);

    browseAndApplyNodes(startNodeId, browser);
}

void OpcUaNodeVisitor::browseAndApplyNodes(const OpcUaNodeId& startNodeId, OpcUaBrowser& browser)
{
    try
    {
        auto& references = browser.browse();

        for (auto& ref : references)
            applyNode(ref);
    }
    catch (const OpcUaException& ex)
    {
        throw OpcUaException(ex.getStatusCode(), std::string(ex.what()) + ". Node: " + startNodeId.toString());
    }
}

void OpcUaNodeVisitor::applyNode(const UA_ReferenceDescription& referenceDescription)
{
    switch (referenceDescription.nodeClass)
    {
        case UA_NODECLASS_VARIABLE:
            applyVariable(referenceDescription);
            break;
        case UA_NODECLASS_OBJECT:
            applyObject(referenceDescription);
            break;
        case UA_NODECLASS_METHOD:
            applyMethod(referenceDescription);
            break;
        case UA_NODECLASS_UNSPECIFIED:
        case UA_NODECLASS_OBJECTTYPE:
        case UA_NODECLASS_VARIABLETYPE:
        case UA_NODECLASS_REFERENCETYPE:
        case UA_NODECLASS_DATATYPE:
        case UA_NODECLASS_VIEW:
        default:
            break;
    }
}

void OpcUaNodeVisitor::applyVariable(const UA_ReferenceDescription& referenceDescription)
{
    traverse(referenceDescription);
}

void OpcUaNodeVisitor::applyObject(const UA_ReferenceDescription& referenceDescription)
{
    traverse(referenceDescription);
}

void OpcUaNodeVisitor::applyMethod(const UA_ReferenceDescription& referenceDescription)
{
    traverse(referenceDescription);
}

OpcUaNodeClass OpcUaNodeVisitor::getBrowseMask()
{
    return browseMask;
}

void OpcUaNodeVisitor::setBrowseMask(OpcUaNodeClass nodeClassMask)
{
    this->browseMask = nodeClassMask;
}

bool OpcUaNodeVisitor::getRecursive()
{
    return recursive;
}

void OpcUaNodeVisitor::setRecursive(bool recursive)
{
    this->recursive = recursive;
}

END_NAMESPACE_OPENDAQ_OPCUA
