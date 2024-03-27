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
#include <opcuashared/opcuadatavalue.h>
#include <opcuashared/opcuanodeid.h>
#include <opcuashared/opcuaobject.h>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

struct OpcUaReadValueId : public OpcUaObject<UA_ReadValueId>
{
public:
    using AttributeIdType = decltype(UA_ReadValueId::attributeId);
};

struct OpcUaReadValueIdWithCallback : public OpcUaReadValueId
{
    using ProcessFunctionType = std::function<void(const OpcUaDataValue&)>;
    OpcUaReadValueIdWithCallback(const OpcUaNodeId& nodeId,
                                 const ProcessFunctionType& processFunction,
                                 AttributeIdType attributeId = UA_ATTRIBUTEID_VALUE);

    ProcessFunctionType processFunction;
};

END_NAMESPACE_OPENDAQ_OPCUA
