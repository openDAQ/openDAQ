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

#include <opcuashared/node/opcuanode.h>
#include <opcuashared/opcuavariant.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaNodeVariable;
using OpcUaNodeVariablePtr = std::shared_ptr<OpcUaNodeVariable>;

class OpcUaNodeVariable : public OpcUaNode
{
public:
    OpcUaNodeVariable(const OpcUaNodeId& uaNode, const UA_DataType& uaDataType, size_t dimension = 1);
    OpcUaNodeVariable(const OpcUaNodeId& uaNode, const OpcUaNodeId& dataTypeNodeId, size_t dimension = 1);
    OpcUaNodeVariable(const UA_ReferenceDescription& uaNodeDescription, const UA_DataType& uaDataType, size_t dimension = 1);
    OpcUaNodeVariable(const UA_ReferenceDescription& uaNodeDescription, const OpcUaNodeId& dataTypeNodeId, size_t dimension = 1);
    ~OpcUaNodeVariable();

    const OpcUaNodeId getDataTypeNodeId() const;

    OpcUaVariantPtr getVariant();
    void setVariant(const UA_Variant& value);

    size_t getDimension();
    void setDimension(size_t dimension);

protected:
    OpcUaNodeId dataTypeNodeId;
    OpcUaVariantPtr variant;
    size_t dimension;
};

END_NAMESPACE_OPENDAQ_OPCUA
