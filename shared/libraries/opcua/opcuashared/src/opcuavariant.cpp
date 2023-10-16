#include "opcuashared/opcuavariant.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <cctype>
#include <open62541/types_generated_handling.h>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using namespace daq::opcua::utils;

OpcUaVariant::OpcUaVariant()
    : OpcUaObject<UA_Variant>()
{
}

OpcUaVariant::OpcUaVariant(const uint16_t& value)
{
    UA_Variant_setScalarCopy(&this->value, &value, &UA_TYPES[UA_TYPES_UINT16]);
}

OpcUaVariant::OpcUaVariant(const int32_t& value)
    : OpcUaVariant()
{
    UA_Variant_setScalarCopy(&this->value, &value, &UA_TYPES[UA_TYPES_INT32]);
}

OpcUaVariant::OpcUaVariant(const int64_t& value)
    : OpcUaVariant()
{
    UA_Variant_setScalarCopy(&this->value, &value, &UA_TYPES[UA_TYPES_INT64]);
}

OpcUaVariant::OpcUaVariant(const char* value)
    : OpcUaVariant()
{
    UA_String* newString = UA_String_new();
    *newString = UA_STRING_ALLOC(value);
    UA_Variant_setScalar(&this->value, newString, &UA_TYPES[UA_TYPES_STRING]);
}

OpcUaVariant::OpcUaVariant(const double& value)
    : OpcUaVariant()
{
    UA_Variant_setScalarCopy(&this->value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
}

OpcUaVariant::OpcUaVariant(const bool& value)
    : OpcUaVariant()
{
    UA_Variant_setScalarCopy(&this->value, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);
}

OpcUaVariant::OpcUaVariant(const OpcUaNodeId& value)
    : OpcUaVariant()
{
    UA_Variant_setScalarCopy(&this->value, value.getPtr(), &UA_TYPES[UA_TYPES_NODEID]);
}

OpcUaVariant::OpcUaVariant(const UA_DataType* type, size_t dimension)
    : OpcUaVariant()
{
    value.type = type;
    if (dimension > 1)
    {
        value.arrayLength = dimension;
        value.arrayDimensions = static_cast<UA_UInt32*>(UA_Array_new(1, type));
        value.arrayDimensions[0] = UA_UInt32(dimension);
        value.arrayDimensionsSize = 1;
    }
}

OpcUaVariant::OpcUaVariant(const double& genericValue, const UA_DataType& originalType)
    : OpcUaVariant()
{
    UA_StatusCode status = ToUaVariant(genericValue, originalType.typeId, &this->value);
    if (status != UA_STATUSCODE_GOOD)
        throw OpcUaVariableConversionError(status);
}


void OpcUaVariant::setValue(UA_Variant&& value)
{
    OpcUaObject<UA_Variant>::setValue(std::move(value));
}

void OpcUaVariant::setValue(const UA_Variant& value, bool shallowCopy)
{
    OpcUaObject<UA_Variant>::setValue(value, shallowCopy);
}

bool OpcUaVariant::isInteger() const
{
    return OpcUaVariant::IsInteger(this->value);
}

bool OpcUaVariant::isString() const
{
    return VariantUtils::HasScalarType<UA_String>(value) ||
           VariantUtils::HasScalarType<UA_LocalizedText>(value);
}

bool OpcUaVariant::isDouble() const
{
    return VariantUtils::HasScalarType<UA_Double>(value);
}

bool OpcUaVariant::isBool() const
{
    return VariantUtils::HasScalarType<UA_Boolean>(value);
}

bool OpcUaVariant::isNodeId() const
{
    return VariantUtils::HasScalarType<UA_NodeId>(value);
}

bool OpcUaVariant::isNull() const
{
    return UA_Variant_isEmpty(&value);
}

bool OpcUaVariant::isReal() const
{
    if (value.type == NULL)
        return false;

    switch (value.type->typeKind)
    {
        case UA_TYPES_FLOAT:
        case UA_TYPES_DOUBLE:
            return true;
        default:
            return false;
    }
}

bool OpcUaVariant::isNumber() const
{
    return isInteger() || isReal();
}

bool OpcUaVariant::IsInteger(const UA_Variant& value)
{
    if (value.type && value.type->typeId.namespaceIndex == 0)  // built-in types
    {
        switch (value.type->typeKind)
        {
            case UA_TYPES_SBYTE:
            case UA_TYPES_BYTE:
            case UA_TYPES_INT16:
            case UA_TYPES_UINT16:
            case UA_TYPES_INT32:
            case UA_TYPES_UINT32:
            case UA_TYPES_INT64:
            case UA_TYPES_UINT64:
                return true;
            default:
                return false;
        }
    }
    return false;
}

std::string OpcUaVariant::toString() const
{
    if (isType<UA_LocalizedText>())
    {
        UA_LocalizedText localizedText = readScalar<UA_LocalizedText>();
        return ToStdString(localizedText.text);
    }

    if (isType<UA_QualifiedName>())
    {
        UA_QualifiedName localizedText = readScalar<UA_QualifiedName>();
        return ToStdString(localizedText.name);
    }

    UA_String str = readScalar<UA_String>();
    return ToStdString(str);
}

int64_t OpcUaVariant::toInteger() const
{
    return VariantUtils::ToNumber(this->value);
}

double OpcUaVariant::toDouble() const
{
    return readScalar<UA_Double>();
}

bool OpcUaVariant::toBool() const
{
    return readScalar<UA_Boolean>();
}

OpcUaNodeId OpcUaVariant::toNodeId() const
{
    return VariantUtils::ToNodeId(this->value);
}

// VariantUtils

void VariantUtils::ToInt32Variant(OpcUaVariant& variant)
{
    if (!variant.isNumber())
        throw OpcUaException(UA_STATUSCODE_BADTYPEMISMATCH, "Variant does not contain a numeric type.");

    UA_Int32 value = (UA_Int32) variant.toInteger();
    variant.setScalar<UA_Int32>(value);
}

void VariantUtils::ToInt64Variant(OpcUaVariant& variant)
{
    if (!variant.isNumber())
        throw OpcUaException(UA_STATUSCODE_BADTYPEMISMATCH, "Variant does not contain a numeric type.");

    UA_Int64 value = (UA_Int64) variant.toInteger();
    variant.setScalar<UA_Int64>(value);
}

END_NAMESPACE_OPENDAQ_OPCUA
