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
#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

struct OpcUaCallMethodRequest : public OpcUaObject<UA_CallMethodRequest>
{
    using OpcUaObject<UA_CallMethodRequest>::OpcUaObject;

public:
    OpcUaCallMethodRequest(const OpcUaNodeId& methodId,
                           const OpcUaNodeId& objectId,
                           size_t inputArgumentsSize = 0,
                           UA_Variant* inputArguments = nullptr);
};

struct OpcUaCallMethodRequestWithCallback : public OpcUaCallMethodRequest
{
public:
    using ProcessFunctionType = std::function<void(const UA_CallMethodResult&)>;
    OpcUaCallMethodRequestWithCallback(const OpcUaNodeId& methodId,
                                       const OpcUaNodeId& objectId,
                                       const ProcessFunctionType& processFunction,
                                       size_t inputArgumentsSize = 0,
                                       UA_Variant* inputArguments = nullptr);
    ProcessFunctionType processFunction = nullptr;
};

END_NAMESPACE_OPENDAQ_OPCUA
