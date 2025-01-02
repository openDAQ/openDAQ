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

/*
 * Abstract representation of OPC UA node (can be object, variable, method ietc.)
 */

#pragma once

#include "opcuashared/opcua.h"
#include "opcuashared/opcuanodeid.h"
#include "opcuashared/opcuacommon.h"
#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaType;
using OpcUaTypePtr = std::shared_ptr<OpcUaType>;

class OpcUaNode;
using OpcUaNodePtr = std::shared_ptr<OpcUaNode>;

class OpcUaNode
{
public:
    OpcUaNode(const OpcUaNodeId& uaNode, OpcUaNodeClass nodeClass);
    OpcUaNode(const UA_ReferenceDescription& uaNodeDescription, OpcUaNodeClass nodeClass);
    virtual ~OpcUaNode();

    // NodeID
    const OpcUaNodeId& getNodeId() const;

    // Attributes
    const OpcUaNodeClass& getNodeClass() const;
    void setNodeClass(const UA_NodeClass& nodeClass);
    void setNodeClass(OpcUaNodeClass nodeClass);
    const std::string& getBrowseName() const;
    void setBrowseName(const std::string& browseName);
    const std::string& getDisplayName() const;
    void setDisplayName(const std::string& displayName);

    static std::string GetBrowseName(const UA_QualifiedName& browseName);

    void setType(const OpcUaTypePtr& type);
    void setType(const OpcUaNodeId& objectTypeId);
    const OpcUaNodeId& getTypeId() const;

protected:
    OpcUaNodeId nodeId;
    std::string browseName;
    std::string displayName;
    OpcUaNodeClass nodeClass;
    OpcUaNodeId typeId;  // hasTypeDefinition (object and variable class)
};

END_NAMESPACE_OPENDAQ_OPCUA
