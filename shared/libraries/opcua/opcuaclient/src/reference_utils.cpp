#include "opcuaclient/reference_utils.h"
#include "opcuaclient/browser/opcuabrowser.h"
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

ReferenceUtils::ReferenceUtils(const OpcUaClientPtr& client)
    : client(client)
{
}

const ReferenceUtils::ReferenceMap& ReferenceUtils::getReferences(const OpcUaNodeId& nodeId)
{
    browseNodeReferences(nodeId);
    return referenceCache[nodeId];
}

tsl::ordered_set<OpcUaNodeId> ReferenceUtils::getReferencedNodes(const OpcUaNodeId& nodeId,
                                                                 const OpcUaNodeId& referenceTypeId,
                                                                 bool isForward,
                                                                 const OpcUaNodeId& typeDefinition)
{
    browseNodeReferences(nodeId);
    tsl::ordered_set<OpcUaNodeId> nodeIds;
    const auto& references = referenceCache[nodeId];
    for (const auto& [refNodeId, ref] : references)
    {
        auto refId = OpcUaNodeId(ref->referenceTypeId);
        if (ref->isForward == isForward && refId == referenceTypeId &&
            (typeDefinition.isNull() || isInstanceOf(ref->typeDefinition.nodeId, typeDefinition)))
            nodeIds.insert(OpcUaNodeId(ref->nodeId.nodeId));
    }
    return nodeIds;
}

bool ReferenceUtils::hasReference(const OpcUaNodeId& nodeId, const std::string& browseName)
{
    browseNodeReferences(nodeId);
    return browseNameCache[nodeId].count(browseName) > 0;
}

bool ReferenceUtils::isInstanceOf(const OpcUaNodeId& typeInQuestion, const OpcUaNodeId& baseType)
{
    if (typeInQuestion == baseType)
        return true;

    browseNodeReferences(baseType);
    if (referenceCache[baseType].count(typeInQuestion) > 0)
    {
        const auto& ref = referenceCache[baseType][typeInQuestion];
        return isHasSubType(ref);
    }

    return false;
}

tsl::ordered_set<OpcUaNodeId> ReferenceUtils::getVariableNodes(const OpcUaNodeId& nodeId)
{
    browseNodeReferences(nodeId);
    tsl::ordered_set<OpcUaNodeId> variableNodes;

    const auto& references = referenceCache[nodeId];
    for (const auto& [refNodeId, ref] : references)
    {
        if (ref->nodeClass == UA_NODECLASS_VARIABLE)
            variableNodes.insert(refNodeId);
    }

    return variableNodes;
}

OpcUaNodeId ReferenceUtils::getChildNodeId(const OpcUaNodeId& nodeId, const std::string& browseName)
{
    browseNodeReferences(nodeId);
    return browseNameCache[nodeId][browseName];
}

void ReferenceUtils::clearCache()
{
    referenceCache.clear();
    browseNameCache.clear();
}

void ReferenceUtils::updateReferences(const OpcUaNodeId& nodeId)
{
    auto browser = OpcUaBrowser(nodeId, client);
    auto references = browser.browse();
    referenceCache[nodeId] = browser.referencesByNodeId();
    buildBrowseNameMap(nodeId);
}

void ReferenceUtils::browseNodeReferences(const OpcUaNodeId& nodeId)
{
    if (referenceCache.count(nodeId) > 0)
        return;

    updateReferences(nodeId);
}

void ReferenceUtils::buildBrowseNameMap(const OpcUaNodeId& nodeId)
{
    const auto& referenceMap = referenceCache[nodeId];
    BrowseNameMap browseNameMap;

    for (const auto& [refNodeId, ref] : referenceMap)
    {
        const auto browseName = getBrowseName(ref);
        browseNameMap.insert({browseName, refNodeId});
    }

    browseNameCache.insert({nodeId, browseNameMap});
}

std::string ReferenceUtils::getBrowseName(const OpcUaObject<UA_ReferenceDescription>& reference)
{
    return utils::ToStdString(reference->browseName.name);
}

bool ReferenceUtils::isHasSubType(const OpcUaObject<UA_ReferenceDescription>& reference)
{
    return OpcUaNodeId(reference->referenceTypeId) == OpcUaNodeId(UA_NS0ID_HASSUBTYPE);
}

END_NAMESPACE_OPENDAQ_OPCUA
