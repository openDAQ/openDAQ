/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <opcuashared/opcuaobject.h>
#include <unordered_map>

template <>
struct std::hash<daq::opcua::OpcUaObject<UA_QualifiedName>>
{
    std::size_t operator()(daq::opcua::OpcUaObject<UA_QualifiedName> const& s) const noexcept
    {
        return UA_QualifiedName_hash(s.get());
    }
};

inline bool operator==(const daq::opcua::OpcUaObject<UA_QualifiedName>& lhs,
                const daq::opcua::OpcUaObject<UA_QualifiedName>& rhs)
{
    return UA_QualifiedName_equal(lhs.get(), rhs.get());
}

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class EventAttributes
{
public:
    EventAttributes();

    using AttributesType = std::unordered_map<OpcUaObject<UA_QualifiedName>, OpcUaObject<UA_Variant>>;

    const AttributesType& getAttributes() const;

    void setTime(UA_UtcTime time);
    void setSeverity(UA_UInt16 eventSeverity);

    void setMessage(const std::string& message);
    void setMessage(const char *locale, const char *text);
    void setMessage(const OpcUaObject<UA_LocalizedText>& message);
    void setSourceName(const std::string& eventSource);

    void setAttribute(const OpcUaObject<UA_QualifiedName>& attribute, const OpcUaObject<UA_Variant>& value);
    void setAttribute(const std::string& attribute, const OpcUaObject<UA_Variant>& value);

private:
    AttributesType attributes;
};

END_NAMESPACE_OPENDAQ_OPCUA
