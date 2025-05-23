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

#include "opcuashared/node/opcuanode.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaNodeObject;
using OpcUaNodeObjectPtr = std::shared_ptr<OpcUaNodeObject>;

class OpcUaNodeObject : public OpcUaNode
{
public:
    explicit OpcUaNodeObject(const OpcUaNodeId& uaNode);
    explicit OpcUaNodeObject(const UA_ReferenceDescription& uaNodeDescription);
    ~OpcUaNodeObject();

    static OpcUaNodeObjectPtr instantiateRoot();
    static OpcUaNodeObjectPtr instantiateObjectsFolder();

private:
};

END_NAMESPACE_OPENDAQ_OPCUA
