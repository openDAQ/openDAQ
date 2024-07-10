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
#include <opcuashared/opcua.h>
#include <open62541/types_generated.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

template <typename Type>
struct TypeToUaDataType
{
    constexpr static const UA_DataType* DataType = nullptr;
};

template <typename Type>
inline constexpr const UA_DataType* GetUaDataType()
{
    static_assert(TypeToUaDataType<Type>::DataType != nullptr, "Implement specialization of TypeToUaDataType");
    return TypeToUaDataType<Type>::DataType;
}

#define ADD_STANDARD_TYPE_MAPPING(TYPE, TYPE_INDEX)                           \
    template <>                                                               \
    struct TypeToUaDataType<TYPE>                                             \
    {                                                                         \
        constexpr static const UA_DataType* DataType = &UA_TYPES[TYPE_INDEX]; \
    };

#define ADD_CUSTOM_TYPE_MAPPING(TYPE, DATA_TYPE)                  \
    template <>                                                   \
    struct TypeToUaDataType<TYPE>                                 \
    {                                                             \
        constexpr static const UA_DataType* DataType = DATA_TYPE; \
    };

#define ADD_STANDARD_TYPE_ALIAS_MAPPING(MAPPING_NAME, TYPE_INDEX)             \
    struct MAPPING_NAME                                                       \
    {                                                                         \
        constexpr static const UA_DataType* DataType = &UA_TYPES[TYPE_INDEX]; \
    };

template <typename UATYPE>
class OpcUaObject
{
public:
    OpcUaObject();
    OpcUaObject(const OpcUaObject& other);
    OpcUaObject(const UATYPE& value, bool shallowCopy = false);
    OpcUaObject(UATYPE&& value);

    virtual ~OpcUaObject();

    inline constexpr const UA_DataType* getUaDataType()
    {
        return GetUaDataType<UATYPE>();
    }

    OpcUaObject& operator=(const OpcUaObject& other);
    OpcUaObject& operator=(OpcUaObject&& other);

    void setValue(UATYPE&& value);
    void setValue(const UATYPE& value, bool shallowCopy = false);

    const UATYPE& getValue() const noexcept;
    UATYPE& getValue() noexcept;

    const UATYPE& operator*() const noexcept;
    UATYPE& operator*() noexcept;

    const UATYPE* get() const noexcept;
    UATYPE* get() noexcept;

    const UATYPE* operator->() const noexcept;
    UATYPE* operator->() noexcept;

    void clear() noexcept;
    UATYPE getDetachedValue() noexcept;

    OpcUaObject<UATYPE> copy() const noexcept;
    UATYPE copyAndGetDetachedValue() const noexcept;

    UATYPE* newDetachedPointer();
    UATYPE* newDetachedPointerCopy() const;

    void markDetached(bool detached);

protected:
    UATYPE value;
    bool shallowCopy = false;
};

template <typename UATYPE>
OpcUaObject<UATYPE>::OpcUaObject()
    : shallowCopy(false)
{
    UA_init(&value, getUaDataType());
}

template <typename UATYPE>
OpcUaObject<UATYPE>::OpcUaObject(const OpcUaObject& other)
    : OpcUaObject<UATYPE>()
{
    setValue(other.getValue());
}

template <typename UATYPE>
OpcUaObject<UATYPE>::OpcUaObject(const UATYPE& value, bool shallowCopy)
    : OpcUaObject<UATYPE>()
{
    setValue(value, shallowCopy);
}

template <typename UATYPE>
OpcUaObject<UATYPE>::OpcUaObject(UATYPE&& value)
    : OpcUaObject<UATYPE>()
{
    setValue(std::forward<decltype(value)>(value));
    UA_init(&value, getUaDataType());
}

template <typename UATYPE>
OpcUaObject<UATYPE>::~OpcUaObject()
{
    clear();
}

template <typename UATYPE>
OpcUaObject<UATYPE>& OpcUaObject<UATYPE>::operator=(const OpcUaObject& other)
{
    if (&other == this)
        return *this;

    setValue(other.getValue());
    return *this;
}

template <typename UATYPE>
OpcUaObject<UATYPE>& OpcUaObject<UATYPE>::operator=(OpcUaObject&& other)
{
    clear();

    value = other.value;
    UA_init(&other.value, getUaDataType());

    shallowCopy = other.shallowCopy;
    return *this;
}

template <typename UATYPE>
void OpcUaObject<UATYPE>::setValue(UATYPE&& value)
{
    clear();
    this->value = value;
    UA_init(&value, getUaDataType());
}

template <typename UATYPE>
void OpcUaObject<UATYPE>::setValue(const UATYPE& value, bool shallowCopy)
{
    clear();
    UA_init(&this->value, getUaDataType());
    if (shallowCopy)
        this->value = value;
    else
        UA_copy(&value, &this->value, getUaDataType());
    this->shallowCopy = shallowCopy;
}

template <typename UATYPE>
const UATYPE& OpcUaObject<UATYPE>::getValue() const noexcept
{
    return this->value;
}

template <typename UATYPE>
UATYPE& OpcUaObject<UATYPE>::getValue() noexcept
{
    return this->value;
}

template <typename UATYPE>
const UATYPE* OpcUaObject<UATYPE>::get() const noexcept
{
    return &this->value;
}

template <typename UATYPE>
UATYPE* OpcUaObject<UATYPE>::get() noexcept
{
    return &this->value;
}

template <typename UATYPE>
const UATYPE* OpcUaObject<UATYPE>::operator->() const noexcept
{
    return &this->value;
}

template <typename UATYPE>
UATYPE* OpcUaObject<UATYPE>::operator->() noexcept
{
    return &this->value;
}

template <typename UATYPE>
const UATYPE& OpcUaObject<UATYPE>::operator*() const noexcept
{
    return getValue();
}

template <typename UATYPE>
UATYPE& OpcUaObject<UATYPE>::operator*() noexcept
{
    return getValue();
}

template <typename UATYPE>
void OpcUaObject<UATYPE>::clear() noexcept
{
    if (shallowCopy)
        UA_init(&this->value, getUaDataType());
    else
        UA_clear(&this->value, getUaDataType());

    shallowCopy = false;
}

template <typename UATYPE>
UATYPE OpcUaObject<UATYPE>::getDetachedValue() noexcept
{
    UATYPE val = getValue();
    shallowCopy = true;
    clear();
    return val;
}

template <typename UATYPE>
OpcUaObject<UATYPE> OpcUaObject<UATYPE>::copy() const noexcept
{
    return OpcUaObject(value);
}

template <typename UATYPE>
UATYPE OpcUaObject<UATYPE>::copyAndGetDetachedValue() const noexcept
{
    return copy().getDetachedValue();
}

template <typename UATYPE>
inline UATYPE* OpcUaObject<UATYPE>::newDetachedPointer()
{
    UATYPE* value = (UATYPE*) UA_new(TypeToUaDataType<UATYPE>::DataType);
    *value = this->getDetachedValue();
    return value;
}

template <typename UATYPE>
inline UATYPE* OpcUaObject<UATYPE>::newDetachedPointerCopy() const
{
    UATYPE* value = (UATYPE*) UA_new(TypeToUaDataType<UATYPE>::DataType);
    *value = this->copyAndGetDetachedValue();
    return value;
}

template <typename UATYPE>
inline void OpcUaObject<UATYPE>::markDetached(bool detached)
{
    this->shallowCopy = detached;
}

ADD_STANDARD_TYPE_MAPPING(UA_Boolean, UA_TYPES_BOOLEAN)
ADD_STANDARD_TYPE_MAPPING(UA_SByte, UA_TYPES_SBYTE)
ADD_STANDARD_TYPE_MAPPING(UA_Byte, UA_TYPES_BYTE)
ADD_STANDARD_TYPE_MAPPING(UA_Int16, UA_TYPES_INT16)
ADD_STANDARD_TYPE_MAPPING(UA_UInt16, UA_TYPES_UINT16)
ADD_STANDARD_TYPE_MAPPING(UA_Int32, UA_TYPES_INT32)
ADD_STANDARD_TYPE_MAPPING(UA_UInt32, UA_TYPES_UINT32)
ADD_STANDARD_TYPE_MAPPING(UA_Int64, UA_TYPES_INT64)
ADD_STANDARD_TYPE_MAPPING(UA_UInt64, UA_TYPES_UINT64)
ADD_STANDARD_TYPE_MAPPING(UA_Variant, UA_TYPES_VARIANT)
ADD_STANDARD_TYPE_MAPPING(UA_String, UA_TYPES_STRING)
ADD_STANDARD_TYPE_MAPPING(UA_Double, UA_TYPES_DOUBLE)
ADD_STANDARD_TYPE_MAPPING(UA_Float, UA_TYPES_FLOAT)
ADD_STANDARD_TYPE_MAPPING(UA_BrowseRequest, UA_TYPES_BROWSEREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_BrowseResponse, UA_TYPES_BROWSERESPONSE)
ADD_STANDARD_TYPE_MAPPING(UA_BrowseNextRequest, UA_TYPES_BROWSENEXTREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_BrowseNextResponse, UA_TYPES_BROWSENEXTRESPONSE)
ADD_STANDARD_TYPE_MAPPING(UA_BrowseDescription, UA_TYPES_BROWSEDESCRIPTION)
ADD_STANDARD_TYPE_MAPPING(UA_BrowseResult, UA_TYPES_BROWSERESULT)
ADD_STANDARD_TYPE_MAPPING(UA_QualifiedName, UA_TYPES_QUALIFIEDNAME)
ADD_STANDARD_TYPE_MAPPING(UA_LocalizedText, UA_TYPES_LOCALIZEDTEXT)
ADD_STANDARD_TYPE_MAPPING(UA_ReadRequest, UA_TYPES_READREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_ReadResponse, UA_TYPES_READRESPONSE)
ADD_STANDARD_TYPE_MAPPING(UA_CallRequest, UA_TYPES_CALLREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_CallResponse, UA_TYPES_CALLRESPONSE)
ADD_STANDARD_TYPE_MAPPING(UA_NodeId, UA_TYPES_NODEID)
ADD_STANDARD_TYPE_MAPPING(UA_ReferenceDescription, UA_TYPES_REFERENCEDESCRIPTION)
ADD_STANDARD_TYPE_MAPPING(UA_ObjectAttributes, UA_TYPES_OBJECTATTRIBUTES)
ADD_STANDARD_TYPE_MAPPING(UA_MethodAttributes, UA_TYPES_METHODATTRIBUTES)
ADD_STANDARD_TYPE_MAPPING(UA_ObjectTypeAttributes, UA_TYPES_OBJECTTYPEATTRIBUTES)
ADD_STANDARD_TYPE_MAPPING(UA_VariableTypeAttributes, UA_TYPES_VARIABLETYPEATTRIBUTES)
ADD_STANDARD_TYPE_MAPPING(UA_VariableAttributes, UA_TYPES_VARIABLEATTRIBUTES)
ADD_STANDARD_TYPE_MAPPING(UA_DataTypeAttributes, UA_TYPES_DATATYPEATTRIBUTES)
ADD_STANDARD_TYPE_MAPPING(UA_ExpandedNodeId, UA_TYPES_EXPANDEDNODEID)
ADD_STANDARD_TYPE_MAPPING(UA_DataValue, UA_TYPES_DATAVALUE)
ADD_STANDARD_TYPE_MAPPING(UA_Argument, UA_TYPES_ARGUMENT)
ADD_STANDARD_TYPE_MAPPING(UA_ReadValueId, UA_TYPES_READVALUEID)
ADD_STANDARD_TYPE_MAPPING(UA_MonitoredItemCreateRequest, UA_TYPES_MONITOREDITEMCREATEREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_MonitoredItemCreateResult, UA_TYPES_MONITOREDITEMCREATERESULT)
ADD_STANDARD_TYPE_MAPPING(UA_CreateSubscriptionRequest, UA_TYPES_CREATESUBSCRIPTIONREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_CreateSubscriptionResponse, UA_TYPES_CREATESUBSCRIPTIONRESPONSE)
ADD_STANDARD_TYPE_MAPPING(UA_CallMethodRequest, UA_TYPES_CALLMETHODREQUEST)
ADD_STANDARD_TYPE_MAPPING(UA_CallMethodResult, UA_TYPES_CALLMETHODRESULT)
ADD_STANDARD_TYPE_MAPPING(UA_Range, UA_TYPES_RANGE)
ADD_STANDARD_TYPE_MAPPING(UA_ComplexNumberType, UA_TYPES_COMPLEXNUMBERTYPE)
ADD_STANDARD_TYPE_MAPPING(UA_DoubleComplexNumberType, UA_TYPES_DOUBLECOMPLEXNUMBERTYPE)
ADD_STANDARD_TYPE_MAPPING(UA_KeyValuePair, UA_TYPES_KEYVALUEPAIR)
ADD_STANDARD_TYPE_MAPPING(UA_ExtensionObject, UA_TYPES_EXTENSIONOBJECT)
ADD_STANDARD_TYPE_MAPPING(UA_RationalNumber, UA_TYPES_RATIONALNUMBER)
ADD_STANDARD_TYPE_MAPPING(UA_EventFilter, UA_TYPES_EVENTFILTER)
ADD_STANDARD_TYPE_MAPPING(UA_SimpleAttributeOperand, UA_TYPES_SIMPLEATTRIBUTEOPERAND)
ADD_STANDARD_TYPE_MAPPING(UA_BrowsePathResult, UA_TYPES_BROWSEPATHRESULT)
ADD_STANDARD_TYPE_MAPPING(UA_EnumField, UA_TYPES_ENUMFIELD)
ADD_STANDARD_TYPE_MAPPING(UA_EUInformation, UA_TYPES_EUINFORMATION)

ADD_STANDARD_TYPE_ALIAS_MAPPING(UtcTimeTypeToUaDataType, UA_TYPES_UTCTIME);

END_NAMESPACE_OPENDAQ_OPCUA
