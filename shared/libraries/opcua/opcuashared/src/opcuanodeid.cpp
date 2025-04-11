#include "opcuashared/opcuanodeid.h"
#include <open62541/util.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNodeId::OpcUaNodeId()
    : OpcUaObject<UA_NodeId>(UA_NODEID_NULL)
{
}

OpcUaNodeId::OpcUaNodeId(uint32_t identifier)
    : OpcUaNodeId(0, identifier)
{
}

OpcUaNodeId::OpcUaNodeId(uint16_t namespaceIndex, const char* identifier)
    : OpcUaObject<UA_NodeId>(UA_NODEID_STRING_ALLOC(namespaceIndex, identifier))
{
}

OpcUaNodeId::OpcUaNodeId(uint16_t namespaceIndex, const std::string& identifier)
    : OpcUaNodeId(namespaceIndex, identifier.c_str())
{
}

OpcUaNodeId::OpcUaNodeId(uint16_t namespaceIndex, uint32_t identifier)
    : OpcUaObject<UA_NodeId>(UA_NODEID_NUMERIC(namespaceIndex, identifier))
{
}

const UA_NodeId* OpcUaNodeId::getPtr() const noexcept
{
    return &value;
}

UA_NodeId* OpcUaNodeId::getPtr() noexcept
{
    return &value;
}

uint16_t OpcUaNodeId::getNamespaceIndex() const noexcept
{
    return value.namespaceIndex;
}

uint32_t OpcUaNodeId::getIdentifierNumeric() const
{
    return value.identifier.numeric;
}

bool OpcUaNodeId::isNull() const noexcept
{
    return UA_NodeId_isNull(&value);
}

OpcUaIdentifierUniversal OpcUaNodeId::getIdentifier() const
{
    return OpcUaNodeId::getIdentifier(value);
}

OpcUaIdentifierType OpcUaNodeId::getIdentifierType() const
{
    return OpcUaNodeId::getIdentifierType(value.identifierType);
}

std::string OpcUaNodeId::toString() const
{
    std::stringstream ss;
    ss << "(";
    ss << getNamespaceIndex();
    ss << ", ";
    ss << getIdentifier();
    ss << ")";
    return ss.str();
}

OpcUaNodeId OpcUaNodeId::instantiateNode(uint16_t namespaceIndex,
                                         OpcUaIdentifierUniversal identifierUniversal,
                                         OpcUaIdentifierType identifierType)
{
    switch (identifierType)
    {
        case OpcUaIdentifierType::Numeric:
        {
            uint32_t id;
            UA_readNumber((uint8_t*)identifierUniversal.c_str(), identifierUniversal.size(), &id);
            return OpcUaNodeId(namespaceIndex, id);
        }
        case OpcUaIdentifierType::String:
            return OpcUaNodeId(namespaceIndex, identifierUniversal);
        default:
            throw std::runtime_error("Unsupported OpcUaIdentifierType!");
    }
}

OpcUaIdentifierUniversal OpcUaNodeId::getIdentifier(const UA_NodeId& uaNodeId)
{
    switch (getIdentifierType(uaNodeId.identifierType))
    {
        case OpcUaIdentifierType::Numeric:
            return std::to_string(uaNodeId.identifier.numeric);
        case OpcUaIdentifierType::String:
            return utils::ToStdString(uaNodeId.identifier.string);
        case OpcUaIdentifierType::Guid:
            return utils::GuidToString(uaNodeId.identifier.guid);
        default:
            throw std::runtime_error("C Exception: unsupported identifier type!");
    };
}

OpcUaIdentifierType OpcUaNodeId::getIdentifierType(const UA_NodeIdType& identifierType)
{
    switch (identifierType)
    {
        case UA_NODEIDTYPE_NUMERIC:
            return OpcUaIdentifierType::Numeric;
        case UA_NODEIDTYPE_STRING:
            return OpcUaIdentifierType::String;
        case UA_NODEIDTYPE_GUID:
            return OpcUaIdentifierType::Guid;
        case UA_NODEIDTYPE_BYTESTRING:
        default:
            return OpcUaIdentifierType::Undefined;
    };
}

void OpcUaNodeId::SetRandomSeed()
{
    UA_random_seed((UA_UInt64) UA_DateTime_now());
}

OpcUaNodeId OpcUaNodeId::CreateWithRandomGuid()
{
    return OpcUaNodeId(UA_NODEID_GUID(1, UA_Guid_random()));
}

END_NAMESPACE_OPENDAQ_OPCUA
