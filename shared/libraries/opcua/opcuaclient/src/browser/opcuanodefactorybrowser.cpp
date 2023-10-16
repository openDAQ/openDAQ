#include "opcuaclient/browser/opcuanodefactorybrowser.h"
#include <opcuashared/opcuaobject.h>
#include <opcuashared/node/opcuanodeobject.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNodeFactoryBrowser::OpcUaNodeFactoryBrowser(const IOpcUaNodeFactoryPtr& nodeFactory, const OpcUaClientPtr& client)
    : OpcUaNodeVisitor(client)
    , nodeFactory(nodeFactory)
{
}

OpcUaNodeFactoryBrowser::~OpcUaNodeFactoryBrowser()
{
}

void OpcUaNodeFactoryBrowser::browseTree()
{
    auto startNode = OpcUaNodeObject::instantiateRoot();
    browseTree(startNode->getNodeId());
}

void OpcUaNodeFactoryBrowser::browseTree(const OpcUaNodePtr& startNodeFolder)
{
    browseTree(startNodeFolder->getNodeId());
}

void OpcUaNodeFactoryBrowser::browseTree(const OpcUaNodeId& startNodeId)
{
    browse(startNodeId, true, OpcUaNodeClass::All);
}

void OpcUaNodeFactoryBrowser::browse(const OpcUaNodePtr& startNodeFolder)
{
    browse(startNodeFolder->getNodeId());
}

void OpcUaNodeFactoryBrowser::browse(const OpcUaNodeId& startNodeId)
{
    browse(startNodeId, false, OpcUaNodeClass::All);
}

void OpcUaNodeFactoryBrowser::browse(const OpcUaNodePtr& startNodeFolder, bool recursive, OpcUaNodeClass nodeClassMask)
{
    browse(startNodeFolder->getNodeId(), recursive, nodeClassMask);
}

void OpcUaNodeFactoryBrowser::browse(const OpcUaNodeId& startNodeId, bool recursive, OpcUaNodeClass nodeClassMask)
{
    results.clear();
    setBrowseMask(nodeClassMask);
    setRecursive(recursive);
    currentParent = startNodeId;
    OpcUaNodeVisitor::traverse(startNodeId);
}

void OpcUaNodeFactoryBrowser::addToList(const OpcUaNodePtr& node)
{
    if (node)
        results.nodes.push_back(node);
}

void OpcUaNodeFactoryBrowser::traverse(const UA_ReferenceDescription& reference)
{
    auto parent = currentParent;
    currentParent = getOpcUaNodeId(reference);
    OpcUaNodeVisitor::traverse(reference);
    
    currentParent = parent;
}

void OpcUaNodeFactoryBrowser::applyNode(const UA_ReferenceDescription& reference)
{
    bool traverseChild;
    auto node = nodeFactory->instantiateNode(reference, currentParent, traverseChild);

    if (traverseChild)
        OpcUaNodeVisitor::applyNode(reference);

    addToList(node);
}

const OpcUaNodeCollection& OpcUaNodeFactoryBrowser::getNodes() const
{
    return results.nodes;
}

size_t OpcUaNodeFactoryBrowser::getNodesSize()
{
    return results.nodes.size();
}

const IOpcUaNodeFactoryPtr& OpcUaNodeFactoryBrowser::getNodeFactory()
{
    return nodeFactory;
}

END_NAMESPACE_OPENDAQ_OPCUA
