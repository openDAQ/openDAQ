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
#include <coretypes/ordinalobject_impl.h>
#include <coretypes/number.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

template <class V, class Intf>
class NumberImpl : public OrdinalObjectImpl<V, Intf, INumber>
{
public:
    using Super = OrdinalObjectImpl<V, Intf, INumber>;

    NumberImpl(V value);

    // INumber
    ErrCode INTERFACE_FUNC getFloatValue(Float* value) override;
    ErrCode INTERFACE_FUNC getIntValue(Int* value) override;

    // IBaseObject
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;
};

template<class V, class Intf>
NumberImpl<V, Intf>::NumberImpl(V value)
    : Super(value)
{
}

template<class V, class Intf>
ErrCode NumberImpl<V, Intf>::getFloatValue(Float* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = static_cast<Float>(this->value);
    return OPENDAQ_SUCCESS;
}

template<class V, class Intf>
ErrCode NumberImpl<V, Intf>::getIntValue(Int* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = static_cast<Int>(this->value);
    return OPENDAQ_SUCCESS;
}

template <class V, class Intf>
ErrCode NumberImpl<V, Intf>::queryInterface(const IntfID& id, void** intf)
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == Intf::Id)
    {
        *intf = static_cast<Intf*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    if (id == INumber::Id)
    {
        *intf = static_cast<INumber*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    return Super::queryInterface(id, intf);
}

template <class V, class Intf>
ErrCode NumberImpl<V, Intf>::borrowInterface(const IntfID& id, void** intf) const
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == Intf::Id)
    {
        *intf = const_cast<Intf*>(static_cast<const Intf*>(this));

        return OPENDAQ_SUCCESS;
    }

    if (id == INumber::Id)
    {
        *intf = const_cast<INumber*>(static_cast<const INumber*>(this));

        return OPENDAQ_SUCCESS;
    }

    return Super::borrowInterface(id, intf);
}


END_NAMESPACE_OPENDAQ
