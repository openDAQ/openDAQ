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
#include "opcuaserver/common.h"
#include "opcuaserver/opcuaservernode.h"
#include <unordered_map>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class ServerEventManager;
using ServerEventManagerPtr = std::shared_ptr<ServerEventManager>;

using CreatOptionalNodeCallback = std::function<bool(const OpcUaNodeId& nodeId)>;
using DisplayNameChangedCallback = std::function<void(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& name)>;

class ServerEventManager
{
public:
    ServerEventManager(const OpcUaServerPtr& server);
    ServerEventManager(OpcUaServer* server);

    void registerEvents();

    void onCreateOptionalNode(const CreatOptionalNodeCallback& callback);

    void onDisplayNameChanged(const OpcUaNodeId& nodeId, const DisplayNameChangedCallback& callback);
    void removeOnDisplayNameChanged(const OpcUaNodeId& nodeId);

private:
    OpcUaServer* server;
    CreatOptionalNodeCallback createOptionalNodeCallback;
    std::unordered_map<OpcUaNodeId, DisplayNameChangedCallback> displayNameCallbacks;

    UA_Boolean triggerCreateOptionalNode(const UA_NodeId* nodeId);
    void triggerDisplayNameChanged(const UA_NodeId* nodeId, UA_LocalizedText* name);

    static UA_Boolean CreateOptionalNode(UA_Server* server,
                                         const UA_NodeId* sessionId,
                                         void* sessionContext,
                                         const UA_NodeId* sourceNodeId,
                                         const UA_NodeId* targetParentNodeId,
                                         const UA_NodeId* referenceTypeId);

    static void DisplayNameChanged(UA_Server* server, UA_NodeId* nodeId, UA_LocalizedText* newDisplayName);
};

END_NAMESPACE_OPENDAQ_OPCUA
