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
#include <opcuashared/opcua.h>
#include <opcuaclient/opcuaclient.h>
#include <tsl/ordered_set.h>
#include <tsl/ordered_map.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class ReferenceUtils;
using ReferenceUtilsPtr = std::shared_ptr<ReferenceUtils>;

class ReferenceUtils
{
public:
    using ReferenceMap = tsl::ordered_map<OpcUaNodeId, OpcUaObject<UA_ReferenceDescription>>;

    explicit ReferenceUtils(const OpcUaClientPtr& client);

    void updateReferences(const OpcUaNodeId& nodeId);
    const ReferenceMap& getReferences(const OpcUaNodeId& nodeId);
    tsl::ordered_set<OpcUaNodeId> getReferencedNodes(const OpcUaNodeId& nodeId,
                                                     const OpcUaNodeId& referenceTypeId,
                                                     bool isForward,
                                                     const OpcUaNodeId& typeDefinition = OpcUaNodeId());
    bool hasReference(const OpcUaNodeId& nodeId, const std::string& browseName);
    bool isInstanceOf(const OpcUaNodeId& typeInQuestion, const OpcUaNodeId& baseType);
    tsl::ordered_set<OpcUaNodeId> getVariableNodes(const OpcUaNodeId& nodeId);
    OpcUaNodeId getChildNodeId(const OpcUaNodeId& nodeId, const std::string& browseName);
    std::string getBrowseName(const OpcUaObject<UA_ReferenceDescription>& reference);
    void clearCache();

protected:
    using BrowseNameMap = std::unordered_map<std::string, OpcUaNodeId>;

    void browseNodeReferences(const OpcUaNodeId& nodeId);
    void buildBrowseNameMap(const OpcUaNodeId& nodeId);
    bool isHasSubType(const OpcUaObject<UA_ReferenceDescription>& reference);

    OpcUaClientPtr client;
    std::unordered_map<OpcUaNodeId, ReferenceMap> referenceCache;
    std::unordered_map<OpcUaNodeId, BrowseNameMap> browseNameCache;
};

END_NAMESPACE_OPENDAQ_OPCUA
