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

#include <opcuashared/opcuacommon.h>
#include <opcuashared/opcuanodeid.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

struct OpcUaAttribute
{
    OpcUaNodeId nodeId;
    UA_AttributeId attributeId;

    OpcUaAttribute(const OpcUaNodeId& nodeId, UA_AttributeId attributeId)
        : nodeId(nodeId)
        , attributeId(attributeId)
    {
    }

    bool operator==(const OpcUaAttribute& other) const
    {
        return nodeId == other.nodeId && attributeId == other.attributeId;
    }
};

END_NAMESPACE_OPENDAQ_OPCUA

namespace std
{
    template <>
    struct hash<daq::opcua::OpcUaAttribute>
    {
        size_t operator()(const daq::opcua::OpcUaAttribute& attr) const noexcept
        {
            size_t hash = UA_NodeId_hash(attr.nodeId.get());
            return UA_ByteString_hash(hash, (const UA_Byte*) &attr.attributeId, sizeof(UA_AttributeId));
        }
    };
}
