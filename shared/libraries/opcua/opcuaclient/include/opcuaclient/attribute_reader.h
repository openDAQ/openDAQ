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
#include <opcuashared/opcuacommon.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuashared/opcua_attribute.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class AttributeReader;
using AttributeReaderPtr = std::shared_ptr<AttributeReader>;

class AttributeReader
{
public:
    AttributeReader(const OpcUaClientPtr& client, size_t maxBatchSize = 0);

    void addAttribute(const OpcUaAttribute& attribute);
    OpcUaDataValuePtr getValue(const OpcUaNodeId& nodeId, UA_AttributeId attributeId);
    OpcUaDataValuePtr getValue(const OpcUaAttribute& attribute);
    void reset();
    void read();
    const std::vector<OpcUaObject<UA_ReadResponse>>& getResponses();

private:
    using ResultMap = std::unordered_map<OpcUaNodeId, std::unordered_map<UA_UInt32, OpcUaDataValuePtr>>;

    size_t readBatch(size_t startIndex, size_t size);
    void addBatchToResultMap(size_t startIndex, const OpcUaObject<UA_ReadResponse>& response);

    OpcUaClientPtr client;
    std::vector<OpcUaAttribute> attributes;
    std::vector<OpcUaObject<UA_ReadResponse>> responses;
    ResultMap resultMap;
    size_t maxBatchSize = 0;
};

END_NAMESPACE_OPENDAQ_OPCUA
