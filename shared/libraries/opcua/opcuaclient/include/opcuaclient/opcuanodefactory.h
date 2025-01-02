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

#include <opcuashared/opcua.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuaclient/browser/opcuanodevisitor.h>
#include <opcuashared/node/opcuanodemethod.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class IOpcUaNodeFactory;
using IOpcUaNodeFactoryPtr = std::shared_ptr<IOpcUaNodeFactory>;

class IOpcUaNodeFactory
{
public:
    virtual ~IOpcUaNodeFactory() = default;
    virtual OpcUaNodePtr instantiateNode(const UA_ReferenceDescription& reference, const OpcUaNodeId& parentNodeId, bool& traverseChild) = 0;
protected:
    static OpcUaNodeId GetOpcUaNodeId(const UA_ReferenceDescription& referenceDescription);
};

class OpcUaNodeFactory;
using OpcUaNodeFactoryPtr = std::shared_ptr<OpcUaNodeFactory>;

class OpcUaNodeFactory : public IOpcUaNodeFactory
{
public:
    explicit OpcUaNodeFactory(const OpcUaClientPtr& client);

    OpcUaNodePtr instantiateNode(const UA_ReferenceDescription& reference, const OpcUaNodeId& parentNodeId, bool& traverseChild) override;

protected:
    void prepareMethodParams(OpcUaNodeMethodPtr nodeMethod);
    void browseAndApplyMethodParams(OpcUaBrowser& browser, const OpcUaNodeMethodPtr& nodeMethod);
    void applyMethodParam(const UA_ReferenceDescription& reference, const OpcUaNodeMethodPtr& nodeMethod);

    OpcUaClientPtr client;
};

END_NAMESPACE_OPENDAQ_OPCUA
