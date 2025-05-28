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
#include <coretypes/integer.h>
#include <coretypes/baseobject_impl.h>
#include <coretypes/convertible.h>
#include <coretypes/comparable.h>
#include <coretypes/coretype.h>
#include <coretypes/coretype_traits.h>
#include <coretypes/serializable.h>

BEGIN_NAMESPACE_OPENDAQ

template <class V, class Intf, class ... Intfs>
class OrdinalObjectImpl : public ImplementationOf<Intf, IConvertible, ICoreType, IComparable, ISerializable, Intfs...>
{
public:
    using Super = ImplementationOf<Intf, IConvertible, ICoreType, IComparable, ISerializable, Intfs...>;

    OrdinalObjectImpl(V value);

    // IBaseObject
    ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) override;
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    // Intf
    ErrCode INTERFACE_FUNC getValue(V* val) override;
    ErrCode INTERFACE_FUNC equalsValue(V val, Bool* equals) override;

    // IConvertible
    ErrCode INTERFACE_FUNC toFloat(Float* val) override;
    ErrCode INTERFACE_FUNC toInt(Int* val) override;
    ErrCode INTERFACE_FUNC toBool(Bool* val) override;

    // ICoreType
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

    // IComparable
    ErrCode INTERFACE_FUNC compareTo(IBaseObject* obj) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* /*id*/) const override
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
    }

protected:
    V value;
};

template <class V, class Intf, class ... Intfs>
OrdinalObjectImpl<V, Intf, Intfs ...>::OrdinalObjectImpl(const V value)
    : value(value)
{
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.");
    }

    *equal = false;

    Intf* otherObj;
    if (OPENDAQ_SUCCEEDED(other->borrowInterface(Intf::Id, reinterpret_cast<void**>(&otherObj))))
    {
        V otherValue;
        ErrCode err = otherObj->getValue(&otherValue);

        if (OPENDAQ_FAILED(err))
        {
            daqClearErrorInfo();
            return OPENDAQ_SUCCESS;
        }

        *equal = otherValue == this->value;
    }
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::getValue(V* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = value;
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::equalsValue(const V val, Bool* equals)
{
    if (equals == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals parameter must not be null.");

    *equals = value == val;
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::getHashCode(SizeT* hashCode)
{
    OPENDAQ_PARAM_NOT_NULL(hashCode);

#if UINTPTR_MAX == UINT64_MAX
    *hashCode = static_cast<SizeT>(value);
#else
    *hashCode = static_cast<SizeT>(static_cast<uint64_t>(value) | ((static_cast<uint64_t>(value)) >> 32));
#endif
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::toString(CharPtr* str)
{
    OPENDAQ_PARAM_NOT_NULL(str);

    std::ostringstream os;
    CoreTypeHelper<V>::Print(os, value);

    return daqDuplicateCharPtr(os.str().c_str(), str);
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::toFloat(Float* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = static_cast<Float>(value);
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::toInt(Int* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = static_cast<Int>(value);
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::toBool(Bool* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = value;

    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = CoreTypeHelper<V>::GetCoreType();
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::compareTo(IBaseObject* obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    V otherValue;

    Intf* typeObj;
    ErrCode err = obj->borrowInterface(Intf::Id, reinterpret_cast<void**>(&typeObj));
    if (OPENDAQ_FAILED(err))
    {
        IConvertible* convObj;
        err = obj->borrowInterface(IConvertible::Id, reinterpret_cast<void**>(&convObj));
        OPENDAQ_RETURN_IF_FAILED(err);

        err = CoreTypeHelper<V>::FromConvertible(otherValue, convObj);
        OPENDAQ_RETURN_IF_FAILED(err);
    }
    else
    {
        err = typeObj->getValue(&otherValue);
        OPENDAQ_RETURN_IF_FAILED(err);
    }

    if (value > otherValue)
        return  OPENDAQ_GREATER;
    else if (value < otherValue)
        return  OPENDAQ_LOWER;
    else
        return  OPENDAQ_EQUAL;
}

template <class V, class Intf, class ... Intfs>
ErrCode OrdinalObjectImpl<V, Intf, Intfs ...>::serialize(ISerializer* serializer)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

END_NAMESPACE_OPENDAQ
