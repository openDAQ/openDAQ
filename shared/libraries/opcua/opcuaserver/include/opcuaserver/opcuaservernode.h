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

#include <opcuaserver/opcuaaddnodeparams.h>
#include <opcuashared/node/opcuanode.h>
#include <opcuashared/opcua.h>
#include <opcuashared/opcuanodeid.h>
#include <opcuashared/opcuavariant.h>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

// forward declaration
class OpcUaServer;

class OpcUaServerObjectNode;
class OpcUaServerVariableNode;
class OpcUaServerMethodNode;
class OpcUaServerViewNode;

class OpcUaServerDataTypeNode;
class OpcUaServerObjectTypeNode;
class OpcUaServerVariableTypeNode;
class OpcUaServerReferenceTypeNode;
class OpcUaServerEventTypeNode;

class OpcUaServerNode
{
public:
    OpcUaServerNode(OpcUaServer& server, const OpcUaNodeId& nodeId);
    virtual ~OpcUaServerNode() = default;

    OpcUaNodeId getNodeId() const;

    OpcUaNodeClass getNodeClass();
    OpcUaObject<UA_QualifiedName> getBrowseName();

    void setDisplayName(const std::string& displayName);
    OpcUaObject<UA_LocalizedText> getDisplayName();

    std::vector<std::unique_ptr<OpcUaServerNode>> browse(
        const OpcUaNodeId& referenceTypeId,
        bool includeSubtypes = true,
        OpcUaNodeClass nodeClassMask = OpcUaNodeClass::All,
        UA_BrowseDirection browseDirection = UA_BrowseDirection::UA_BROWSEDIRECTION_FORWARD);
    std::vector<std::unique_ptr<OpcUaServerNode>> browseChildNodes();
    std::unique_ptr<OpcUaServerNode> getChildNode(const OpcUaObject<UA_QualifiedName>& qualifiedName);

    OpcUaServerObjectNode addObject(AddObjectNodeParams& params);
    OpcUaServerObjectNode addObject(const OpcUaNodeId& id, const std::string& browseName);
    OpcUaServerVariableNode addVariable(AddVariableNodeParams& params);
    OpcUaServerVariableNode addVariable(const OpcUaNodeId& id, const std::string& browseName);

    void remove();

protected:
    OpcUaServer& server;
    OpcUaNodeId nodeId;
};

class OpcUaServerObjectNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

class OpcUaServerVariableNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors

    template <typename Arg>
    void write(Arg&& arg)
    {
        OpcUaVariant var;
        var.setScalar(std::forward<Arg>(arg));
        writeVariantToServer(var);
    }

    template <typename T>
    T read()
    {
        OpcUaVariant var = readVariantFromServer();
        return var.readScalar<T>();
    }

private:
    void writeVariantToServer(const OpcUaVariant& var);
    OpcUaVariant readVariantFromServer();
};

class OpcUaServerMethodNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

class OpcUaServerViewNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

class OpcUaServerDataTypeNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

class OpcUaServerObjectTypeNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

class OpcUaServerVariableTypeNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

class OpcUaServerReferenceTypeNode : public OpcUaServerNode
{
public:
    using OpcUaServerNode::OpcUaServerNode;  // inherit constructors
};

}  // namespace opcua
