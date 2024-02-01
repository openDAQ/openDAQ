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

#include "opcuacommon.h"
#include <mutex>
#include <opcuashared/opcuanodeid.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaVariant;
using OpcUaVariantPtr = std::shared_ptr<OpcUaVariant>;

namespace VariantUtils
{
    inline bool IsScalar(const UA_Variant& value)
    {
        return UA_Variant_isScalar(&value);
    }

    inline bool IsVector(const UA_Variant& value)
    {
        return !IsScalar(value);
    }

    template <typename T>
    inline bool IsType(const UA_Variant& value)
    {
        return value.type == GetUaDataType<T>();
    }

    template <typename T>
    inline bool HasScalarType(const UA_Variant& value)
    {
        return IsScalar(value) && IsType<T>(value);
    }

    template <typename T>
    inline T ReadScalar(const UA_Variant& value)
    {
        if (!IsScalar(value))
            throw std::runtime_error("Variant is not a scalar");
        if (!IsType<T>(value))
            throw std::runtime_error("Variant does not contain a scalar of specified return type");
        return *static_cast<T*>(value.data);
    }

    inline std::string ToString(const UA_Variant& value)
    {
        return utils::ToStdString(ReadScalar<UA_String>(value));
    }

    inline int64_t ToNumber(const UA_Variant& value)
    {
        switch (value.type->typeKind)
        {
            case UA_TYPES_SBYTE:
                return ReadScalar<UA_SByte>(value);
            case UA_TYPES_BYTE:
                return ReadScalar<UA_Byte>(value);
            case UA_TYPES_INT16:
                return ReadScalar<UA_Int16>(value);
            case UA_TYPES_UINT16:
                return ReadScalar<UA_UInt16>(value);
            case UA_TYPES_INT32:
                return ReadScalar<UA_Int32>(value);
            case UA_TYPES_UINT32:
                return ReadScalar<UA_UInt32>(value);
            case UA_TYPES_INT64:
                return ReadScalar<UA_Int64>(value);
            case UA_TYPES_UINT64:
                return ReadScalar<UA_UInt64>(value);
            default:
                throw std::runtime_error("Type not supported!");
        }
    }

    inline OpcUaNodeId ToNodeId(const UA_Variant& value)
    {
        UA_NodeId nodeId = ReadScalar<UA_NodeId>(value);
        return OpcUaNodeId(nodeId);
    }

    void ToInt32Variant(OpcUaVariant& variant);
    void ToInt64Variant(OpcUaVariant& variant);
}

class OpcUaVariant : public OpcUaObject<UA_Variant>
{
public:
    using OpcUaObject<UA_Variant>::OpcUaObject;

    OpcUaVariant();

    explicit OpcUaVariant(const uint16_t& value);
    explicit OpcUaVariant(const uint32_t& value);
    explicit OpcUaVariant(const int32_t& value);
    explicit OpcUaVariant(const int64_t& value);
    explicit OpcUaVariant(const char* value);
    explicit OpcUaVariant(const double& value);
    explicit OpcUaVariant(const bool& value);
    explicit OpcUaVariant(const OpcUaNodeId& value);
    OpcUaVariant(const UA_DataType* type, size_t dimension);
    OpcUaVariant(const double& genericValue, const UA_DataType& originalType);

    template <typename T>
    inline bool isType() const
    {
        return VariantUtils::IsType<T>(this->value);
    }

    template <typename T>
    inline bool hasScalarType()
    {
        return VariantUtils::HasScalarType<T>(this->value);
    }

    template <typename T>
    T readScalar() const
    {
        return VariantUtils::ReadScalar<T>(this->value);
    }

    template <typename T, typename UATYPE = TypeToUaDataType<T>>
    void setScalar(const T& value)
    {
        static_assert(UATYPE::DataType != nullptr, "Implement specialization of TypeToUaDataType");

        this->clear();

        const auto status = UA_Variant_setScalarCopy(&this->value, &value, UATYPE::DataType);
        CheckStatusCodeException(status);
    }

    void setValue(UA_Variant&& value);
    void setValue(const UA_Variant& value, bool shallowCopy = false);

    bool isInteger() const;
    bool isString() const;
    bool isDouble() const;
    bool isBool() const;
    bool isNodeId() const;
    bool isNull() const;
    bool isReal() const;
    bool isNumber() const;

    std::string toString() const;
    int64_t toInteger() const;
    double toDouble() const;
    float toFloat() const;
    bool toBool() const;
    OpcUaNodeId toNodeId() const;

    inline bool isScalar() const
    {
        return VariantUtils::IsScalar(value);
    }
    inline bool isVector() const
    {
        return VariantUtils::IsVector(value);
    }

    static bool IsInteger(const UA_Variant& value);
};

class OpcUaVariableConversionError : public OpcUaException
{
public:
    OpcUaVariableConversionError(UA_StatusCode statusCode)
        : OpcUaException(statusCode, "Conversion error")
    {
    }
};

END_NAMESPACE_OPENDAQ_OPCUA
