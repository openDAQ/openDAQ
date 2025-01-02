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
#include <opcuashared/opcuanodeid.h>
#include <open62541/nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

struct BrowseRequest : public OpcUaObject<UA_BrowseRequest>
{
    using OpcUaObject<UA_BrowseRequest>::OpcUaObject;

public:

    explicit BrowseRequest(size_t nodesToBrowseSize);
    BrowseRequest(const OpcUaNodeId& nodeId, OpcUaNodeClass nodeClassMask = OpcUaNodeClass::All, const OpcUaNodeId& referenceTypeId = OpcUaNodeId(UA_NS0ID_REFERENCES), const OpcUaBrowseDirection browseDirection = OpcUaBrowseDirection::Forward);

    void resizeNodesToBrowse(size_t newNodesToBrowseSize);
};


END_NAMESPACE_OPENDAQ_OPCUA
