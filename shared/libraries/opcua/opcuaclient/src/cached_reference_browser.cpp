#include <opcuaclient/cached_reference_browser.h>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

CachedReferenceBrowser::CachedReferenceBrowser(const OpcUaClientPtr& client, size_t maxNodesPerBrowse)
    : client(client)
    , maxNodesPerBrowse(maxNodesPerBrowse)
{
}

const CachedReferences& CachedReferenceBrowser::browse(const OpcUaNodeId& nodeId, bool forceInvalidate)
{
    if (forceInvalidate)
        invalidate(nodeId);

    if (!isCached(nodeId))
        browseMultiple({nodeId});

    return references[nodeId];
}

void CachedReferenceBrowser::invalidate(const OpcUaNodeId& nodeId)
{
    if (!isCached(nodeId))
        return;

    const auto children = references[nodeId].byNodeId;
    references.erase(nodeId);

    for (const auto& [refNodeId, ref] : children)
    {
        if (ref->isForward)
            invalidate(refNodeId);
    }
}

bool CachedReferenceBrowser::isSubtypeOf(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType)
{
    if (typeId == baseType)
        return true;

    browse(baseType);

    for (const auto& [refNodeId, ref] : references[baseType].byNodeId)
    {
        if (OpcUaNodeId(ref->referenceTypeId) == OpcUaNodeId(UA_NS0ID_HASSUBTYPE) && isSubtypeOf(typeId, refNodeId))
            return true;
    }

    return false;
}

bool CachedReferenceBrowser::hasReference(const OpcUaNodeId& nodeId, const std::string& browseName)
{
    browse(nodeId);
    return references[nodeId].byBrowseName.count(browseName) > 0;
}

OpcUaNodeId CachedReferenceBrowser::getChildNodeId(const OpcUaNodeId& nodeId, const std::string& browseName)
{
    browse(nodeId);
    const auto& node = references[nodeId].byBrowseName[browseName];
    return node->nodeId.nodeId;
}

CachedReferences CachedReferenceBrowser::browseFiltered(const OpcUaNodeId& nodeId, const BrowseFilter& filter)
{
    const auto& references = browse(nodeId);
    CachedReferences filtered;

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        auto typeId = OpcUaNodeId(ref->referenceTypeId);

        if (!filter.referenceTypeId.isNull() && filter.referenceTypeId != typeId)
            continue;
        if (!filter.typeDefinition.isNull() && !isSubtypeOf(ref->typeDefinition.nodeId, filter.typeDefinition))
            continue;
        if (filter.direction == UA_BROWSEDIRECTION_FORWARD && !ref->isForward)
            continue;
        if (filter.direction == UA_BROWSEDIRECTION_INVERSE && ref->isForward)
            continue;

        filtered.byNodeId.insert({ref->nodeId.nodeId, ref});
        filtered.byBrowseName.insert({browseName, ref});
    }

    return filtered;
}

bool CachedReferenceBrowser::isCached(const OpcUaNodeId& nodeId)
{
    return references.count(nodeId) > 0;
}

void CachedReferenceBrowser::markAsCached(const OpcUaNodeId& nodeId)
{
    if (references.count(nodeId) == 0)
        references[nodeId] = {};
}

void CachedReferenceBrowser::browseMultiple(const std::vector<OpcUaNodeId>& nodes)
{
    const size_t batchSize = (maxNodesPerBrowse > 0) ? maxNodesPerBrowse : nodes.size();
    size_t i = 0;

    while (i < nodes.size())
        i += browseBatch(nodes, i, batchSize);
}

size_t CachedReferenceBrowser::browseBatch(const std::vector<OpcUaNodeId>& nodes, size_t startIndex, size_t size)
{
    if ((startIndex + size) > nodes.size())
        size = nodes.size() - startIndex;

    assert(size > 0);

    OpcUaObject<UA_BrowseRequest> request;
    request->requestedMaxReferencesPerNode = 0;
    request->nodesToBrowse = (UA_BrowseDescription*) UA_Array_new(size, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION]);
    request->nodesToBrowseSize = size;

    for (size_t i = 0; i < size; i++)
    {
        const auto& nodeId = nodes[startIndex + i];
        markAsCached(nodeId);

        request->nodesToBrowse[i].nodeId = nodeId.copyAndGetDetachedValue();
        request->nodesToBrowse[i].resultMask = UA_BROWSERESULTMASK_ALL;
        request->nodesToBrowse[i].browseDirection = UA_BROWSEDIRECTION_FORWARD;
    }

    std::vector<OpcUaNodeId> browseNext;
    UA_ByteString* continuationPoint = nullptr;
    OpcUaObject<UA_BrowseResponse> response = UA_Client_Service_browse(client->getLockedUaClient(), *request);
    CheckStatusCodeException(response->responseHeader.serviceResult, "Browse result error");

    processBrowseResults(nodes, startIndex, size, response->results, response->resultsSize, browseNext);

    while (getContinuationPoint(response->results, continuationPoint))
    {
        OpcUaObject<UA_BrowseNextRequest> nextRequest;
        nextRequest->releaseContinuationPoints = UA_FALSE;
        nextRequest->continuationPointsSize = 1u;
        nextRequest->continuationPoints = continuationPoint;
        OpcUaObject<UA_BrowseNextResponse> nextResponse = UA_Client_Service_browseNext(client->getLockedUaClient(), *nextRequest);
        CheckStatusCodeException(response->responseHeader.serviceResult, "Browse result error");

        processBrowseResults(nodes, startIndex, size, nextResponse->results, nextResponse->resultsSize, browseNext);
    }

    browseMultiple(browseNext);
    return size;
}

void CachedReferenceBrowser::processBrowseResults(const std::vector<OpcUaNodeId>& nodes,
                                                  size_t startIndex,
                                                  size_t requestedSize,
                                                  UA_BrowseResult* results,
                                                  size_t resultSize,
                                                  std::vector<OpcUaNodeId>& browseNextOut)
{
    assert(requestedSize == resultSize);

    for (size_t i = 0; i < resultSize; i++)
    {
        UA_BrowseResult& result = results[i];

        if (result.statusCode == UA_STATUSCODE_BADUSERACCESSDENIED)
            continue;
        if (result.statusCode == UA_STATUSCODE_BADNODEIDUNKNOWN)
            continue;

        CheckStatusCodeException(result.statusCode, "Browse result error");

        const auto nodeId = nodes[startIndex + i];
        references[nodeId] = {};

        for (size_t j = 0; j < result.referencesSize; j++)
        {
            const auto& ref = result.references[j];
            const std::string browseName = utils::ToStdString(ref.browseName.name);
            const auto refId = OpcUaNodeId(ref.nodeId.nodeId);

            references[nodeId].byNodeId.insert({refId, ref});
            references[nodeId].byBrowseName.insert({browseName, ref});

            if (ref.isForward && references.count(refId) == 0)
                browseNextOut.push_back(refId);
        }
    }
}

bool CachedReferenceBrowser::getContinuationPoint(UA_BrowseResult* results, UA_ByteString* continuationPointOut)
{
    continuationPointOut = &results->continuationPoint;
    return results->continuationPoint.length > 0;
}

END_NAMESPACE_OPENDAQ_OPCUA
