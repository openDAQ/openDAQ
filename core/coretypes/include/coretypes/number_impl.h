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
#include <coretypes/ordinalobject_impl.h>
#include <coretypes/number.h>

BEGIN_NAMESPACE_OPENDAQ

template <class V, class Intf>
class NumberImpl : public OrdinalObjectImpl<V, Intf, INumber>
{
public:
    NumberImpl(V value);

    // INumber
    ErrCode INTERFACE_FUNC getFloatValue(Float* value) override;
    ErrCode INTERFACE_FUNC getIntValue(Int* value) override;
};

template<class V, class Intf>
inline NumberImpl<V, Intf>::NumberImpl(V value)
    : OrdinalObjectImpl<V, Intf, INumber>(value)
{
}

template<class V, class Intf>
inline ErrCode INTERFACE_FUNC NumberImpl<V, Intf>::getFloatValue(Float* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = static_cast<Float>(this->value);
    return OPENDAQ_SUCCESS;
}

template<class V, class Intf>
inline ErrCode INTERFACE_FUNC NumberImpl<V, Intf>::getIntValue(Int* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = static_cast<Int>(this->value);
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
