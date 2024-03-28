/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opcuaserver/opcuaservernode.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaServerNodeFactory
{
public:
    OpcUaServerNodeFactory(OpcUaServer& server);

    std::unique_ptr<OpcUaServerNode> createServerNode(const OpcUaNodeId& nodeId);
    std::unique_ptr<OpcUaServerNode> createServerNode(const OpcUaNodeId& nodeId, OpcUaNodeClass nodeClass);
    std::unique_ptr<OpcUaServerNode> createServerNode(const OpcUaNodeId& nodeId, UA_NodeClass nodeClass);

protected:
    OpcUaServer& server;
};

}  // namespace opcua
