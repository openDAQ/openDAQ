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
#include <memory>
#include <opcuashared/opcuacommon.h>
#include <opcuashared/opcuaobject.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaIdentifierUniversal = std::string;

using UA_NodeIdPtr = UA_NodeId*;

enum class OpcUaIdentifierType
{
    Numeric = 0,
    String = 3,
    Guid = 4, 
    Undefined = -1
};

static_assert(static_cast<int>(OpcUaIdentifierType::Numeric) == UA_NODEIDTYPE_NUMERIC);
static_assert(static_cast<int>(OpcUaIdentifierType::String) == UA_NODEIDTYPE_STRING);
static_assert(static_cast<int>(OpcUaIdentifierType::Guid) == UA_NODEIDTYPE_GUID);

#define OPCUANODEID_ROOTFOLDER OpcUaNodeId(0u, UA_NS0ID_ROOTFOLDER)
#define OPCUANODEID_OBJECTSFOLDER OpcUaNodeId(0u, UA_NS0ID_OBJECTSFOLDER)

class OpcUaNodeId : public OpcUaObject<UA_NodeId>
{
    using OpcUaObject<UA_NodeId>::OpcUaObject;

public:
    OpcUaNodeId();
    explicit OpcUaNodeId(uint32_t identifier);
    OpcUaNodeId(uint16_t namespaceIndex, const char* identifier);
    OpcUaNodeId(uint16_t namespaceIndex, const std::string& identifier);
    OpcUaNodeId(uint16_t namespaceIndex, uint32_t identifier);

    inline bool operator==(const OpcUaNodeId& rhs) const noexcept
    {
        return UA_NodeId_equal(this->getPtr(), rhs.getPtr());
    }

    inline bool operator!=(const OpcUaNodeId& rhs) const noexcept
    {
        return !UA_NodeId_equal(this->getPtr(), rhs.getPtr());
    }

    inline bool operator==(const UA_NodeId& rhs) const noexcept
    {
        return UA_NodeId_equal(this->getPtr(), &rhs);
    }

    inline bool operator!=(const UA_NodeId& rhs) const noexcept
    {
        return !UA_NodeId_equal(this->getPtr(), &rhs);
    }

    friend inline bool operator<(const OpcUaNodeId& l, const OpcUaNodeId& r)  noexcept
    {
        const auto& lhs = l.getValue();
        const auto& rhs = r.getValue();
        if (lhs.namespaceIndex != rhs.namespaceIndex)
            return lhs.namespaceIndex < rhs.namespaceIndex;

        if (lhs.identifierType != rhs.identifierType)
            return lhs.identifierType < rhs.identifierType;

        switch (lhs.identifierType)
        {
            case UA_NODEIDTYPE_NUMERIC:
                return lhs.identifier.numeric < rhs.identifier.numeric;
            case UA_NODEIDTYPE_GUID:
                return memcmp(&lhs.identifier.guid, &rhs.identifier.guid, sizeof(UA_Guid)) < 0;
            case UA_NODEIDTYPE_BYTESTRING:
            case UA_NODEIDTYPE_STRING:
                if (lhs.identifier.string.length != rhs.identifier.string.length)
                    return lhs.identifier.string.length < rhs.identifier.string.length;

                return memcmp((char const*) lhs.identifier.string.data,
                              (char const*) rhs.identifier.string.data,
                              lhs.identifier.string.length) < 0;
        }
        return false;
    }

    const UA_NodeId* getPtr() const noexcept;
    UA_NodeId* getPtr() noexcept;

    uint16_t getNamespaceIndex() const noexcept;
    uint32_t getIdentifierNumeric() const;
    OpcUaIdentifierUniversal getIdentifier() const;
    OpcUaIdentifierType getIdentifierType() const;
    std::string toString() const;

    bool isNull() const noexcept;

    static void SetRandomSeed();
    static OpcUaNodeId CreateWithRandomGuid();

    static OpcUaNodeId instantiateNode(uint16_t namespaceIndex,
                                       OpcUaIdentifierUniversal identifierUniversal,
                                       OpcUaIdentifierType identifierType);
    static OpcUaIdentifierUniversal getIdentifier(const UA_NodeId& uaNodeId);

    static OpcUaIdentifierType getIdentifierType(const UA_NodeIdType& identifierType);

    template <typename... Args>
    inline OpcUaNodeId addSuffix(Args... args) const
    {
        std::stringstream ss;
        concat(ss, getIdentifier(), args...);
        return OpcUaNodeId(getNamespaceIndex(), ss.str());
    }

private:

    template <typename T>
    static void concat(std::ostream& out, const T& first)
    {
        out << first;
    }

    template <typename T, typename... Args>
    static void concat(std::ostream& out, const T& first, const Args&... rest)
    {
        out << first;
        concat(out, rest...);
    }
};

END_NAMESPACE_OPENDAQ_OPCUA

namespace std
{
    template <>
    struct hash<daq::opcua::OpcUaNodeId>
    {
        size_t operator()(const daq::opcua::OpcUaNodeId& k) const noexcept
        {
            return static_cast<size_t>(UA_NodeId_hash(k.get()));
        }
    };
}
