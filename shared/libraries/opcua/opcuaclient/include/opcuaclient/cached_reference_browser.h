/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
};

class CachedReferenceBrowser
{
public:
    CachedReferenceBrowser(const OpcUaClientPtr& client);

    const CachedReferences& browse(const OpcUaNodeId& nodeId, bool forceInvalidate = false);
    void invalidate(const OpcUaNodeId& nodeId);

    bool isSubtypeOf(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType);
    bool hasReference(const OpcUaNodeId& nodeId, const std::string& browseName);
    OpcUaNodeId getChildNodeId(const OpcUaNodeId& nodeId, const std::string& browseName);
    CachedReferences browseFiltered(const OpcUaNodeId& nodeId, const BrowseFilter& filter);

private:
    bool isCached(const OpcUaNodeId& nodeId);
    void markAsCached(const OpcUaNodeId& nodeId);
    void browseMultiple(const std::vector<OpcUaNodeId>& nodes);
    void processBrowseResults(const std::vector<OpcUaNodeId>& nodes, UA_BrowseResult* results, size_t size, std::vector<OpcUaNodeId>& browseNextOut);
    bool getContinuationPoint(UA_BrowseResult* results, UA_ByteString* continuationPointOut);

    OpcUaClientPtr client;
    std::unordered_map<OpcUaNodeId, CachedReferences> references;
};

END_NAMESPACE_OPENDAQ_OPCUA
