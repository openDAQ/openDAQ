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
#include "opcuanode.h"
#include <list>

BEGIN_NAMESPACE_OPENDAQ_OPCUA


struct OpcUaChannelMethodParameter
{
public:
    std::string getName() const
    {
        return name;
    }
    void setName(const std::string& name)
    {
        this->name = name;
    }

    OpcUaNodeId getDataTypeId() const
    {
        return dataTypeId;
    }

    void setDataTypeId(const OpcUaNodeId& dataTypeId)
    {
        this->dataTypeId = dataTypeId;
    }

private:
    std::string name;
    OpcUaNodeId dataTypeId;
};

class OpcUaNodeMethod;
using OpcUaNodeMethodPtr = std::shared_ptr<OpcUaNodeMethod>;

class OpcUaNodeMethod : public OpcUaNode
{
public:
    OpcUaNodeMethod(const OpcUaNodeId& uaNode, const OpcUaNodeId& parentNodeId);
    OpcUaNodeMethod(const UA_ReferenceDescription& uaNodeDescription, const OpcUaNodeId& parentNodeId);
    ~OpcUaNodeMethod();

    std::string getTypeDescription();
    void initTypeDescription();

    void addInputParameter(const std::string& name, const OpcUaNodeId& dataTypeId);
    void addOutputParameter(const std::string& name, const OpcUaNodeId& dataTypeId);

    std::list<OpcUaChannelMethodParameter> inputParameters;
    std::list<OpcUaChannelMethodParameter> outputParameters;

    const OpcUaNodeId& getParentNodeId() const;

protected:
    std::string typeDescription;
    OpcUaNodeId parentNodeId;
};

END_NAMESPACE_OPENDAQ_OPCUA
