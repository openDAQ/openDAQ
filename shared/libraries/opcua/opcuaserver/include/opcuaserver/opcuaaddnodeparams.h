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
#include <open62541/server.h>

#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using CreateOptionalNodeCallback = std::function<bool(const OpcUaNodeId& nodeId)>;

class AddNodeParams
{
public:
    AddNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId, const OpcUaNodeId& referenceTypeId);
    void setBrowseName(const std::string& browseName);

    OpcUaNodeId requestedNewNodeId;
    OpcUaNodeId parentNodeId;
    OpcUaNodeId referenceTypeId;
    OpcUaObject<UA_QualifiedName> browseName{};
    void* nodeContext{};
};

template <typename T = UA_GenericAttributes>
class GenericAddNodeParams : public AddNodeParams
{
public:
    GenericAddNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId, const OpcUaNodeId& referenceTypeId, const T& defaultAttributes);

    OpcUaObject<T> attr;
    CreateOptionalNodeCallback addOptionalNodeCallback;
};

class AddObjectNodeParams : public GenericAddNodeParams<UA_ObjectAttributes>
{
public:
    AddObjectNodeParams(const OpcUaNodeId& requestedNewNodeId);
    AddObjectNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId);
    AddObjectNodeParams(const std::string& name, const OpcUaNodeId& parentNodeId);

    OpcUaNodeId typeDefinition = OpcUaNodeId(UA_NS0ID_BASEOBJECTTYPE);
};

class AddVariableNodeParams : public GenericAddNodeParams<UA_VariableAttributes>
{
public:
    AddVariableNodeParams(const OpcUaNodeId& requestedNewNodeId);
    AddVariableNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId);
    AddVariableNodeParams(const std::string& name, const OpcUaNodeId& parentNodeId);

    void setDataType(const OpcUaNodeId& dataTypeId);

    OpcUaNodeId typeDefinition = OpcUaNodeId(UA_NS0ID_BASEDATAVARIABLETYPE);
};

class AddMethodNodeParams : public GenericAddNodeParams<UA_MethodAttributes>
{
public:
    AddMethodNodeParams(const OpcUaNodeId& requestedNewNodeId);
    AddMethodNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId);
    AddMethodNodeParams(const std::string& name, const OpcUaNodeId& parentNodeId);

    ~AddMethodNodeParams();

    UA_MethodCallback method{};
    size_t inputArgumentsSize{};
    UA_Argument* inputArguments{};
    size_t outputArgumentsSize{};
    UA_Argument* outputArguments{};
};

class AddVariableTypeNodeParams : public GenericAddNodeParams<UA_VariableTypeAttributes>
{
public:
    AddVariableTypeNodeParams(const OpcUaNodeId& requestedNewNodeId);
    AddVariableTypeNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId);

    OpcUaNodeId typeDefinition = OpcUaNodeId(UA_NS0ID_BASEDATAVARIABLETYPE);
};

struct AddObjectTypeNodeParams : public GenericAddNodeParams<UA_ObjectTypeAttributes>
{
    AddObjectTypeNodeParams(const OpcUaNodeId& requestedNewNodeId);
    AddObjectTypeNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId);
};

END_NAMESPACE_OPENDAQ_OPCUA
