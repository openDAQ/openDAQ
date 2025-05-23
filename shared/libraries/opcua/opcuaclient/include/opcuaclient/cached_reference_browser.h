/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>
#include <unordered_map>

#include <opcuaclient/browser/opcuabrowser.h>
#include <opcuashared/opcuanodeid.h>
#include <unordered_set>
#include <opcuashared/opcua.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class CachedReferenceBrowser;
using CachedReferenceBrowserPtr = std::shared_ptr<CachedReferenceBrowser>;

struct CachedReferences
{
    tsl::ordered_map<OpcUaNodeId, OpcUaObject<UA_ReferenceDescription>> byNodeId;
    tsl::ordered_map<std::string, OpcUaObject<UA_ReferenceDescription>> byBrowseName;
};

struct BrowseFilter
{
    OpcUaNodeId referenceTypeId = OpcUaNodeId();
    OpcUaNodeId typeDefinition = OpcUaNodeId();
    UA_BrowseDirection direction = UA_BROWSEDIRECTION_BOTH;
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
};

class CachedReferenceBrowser
{
public:
    CachedReferenceBrowser(const OpcUaClientPtr& client, size_t maxNodesPerBrowse = 0);

    const CachedReferences& browse(const OpcUaNodeId& nodeId);
    void browseMultiple(const std::vector<OpcUaNodeId>& nodes);
    void invalidate(const OpcUaNodeId& nodeId);
    void invalidateRecursive(const OpcUaNodeId& nodeId);

    bool isSubtypeOf(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType);
    OpcUaNodeId getTypeDefinition(const OpcUaNodeId& nodeId);
    bool hasReference(const OpcUaNodeId& nodeId, const std::string& browseName);
    OpcUaNodeId getChildNodeId(const OpcUaNodeId& nodeId, const std::string& browseName);
    CachedReferences browseFiltered(const OpcUaNodeId& nodeId, const BrowseFilter& filter);

private:
    void invalidate(const OpcUaNodeId& nodeId, bool recursive);
    bool isCached(const OpcUaNodeId& nodeId);
    void markAsCached(const OpcUaNodeId& nodeId);
    size_t browseBatch(const std::vector<OpcUaNodeId>& nodes, size_t startIndex, size_t size, std::vector<OpcUaNodeId>& browseNext);
    void processBrowseResults(const std::vector<OpcUaNodeId>& nodes,
                              size_t startIndex,
                              size_t requestedSize,
                              UA_BrowseResult* results,
                              size_t resultSize,
                              std::vector<OpcUaNodeId>& browseNextOut);
    bool getContinuationPoint(UA_BrowseResult* results, UA_ByteString** continuationPointOut);

    OpcUaClientPtr client;
    size_t maxNodesPerBrowse;
    std::unordered_map<OpcUaNodeId, CachedReferences> references;
};

END_NAMESPACE_OPENDAQ_OPCUA
