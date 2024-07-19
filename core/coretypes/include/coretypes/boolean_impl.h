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
#include <coretypes/boolean.h>
#include <coretypes/ordinalobject_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <>
inline ErrCode OrdinalObjectImpl<Bool, IBoolean>::toBool(Bool* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (value != 0)
        *val = True;
    else
        *val = False;

    return OPENDAQ_SUCCESS;
}

template <>
inline ErrCode OrdinalObjectImpl<Bool, IBoolean>::getHashCode(SizeT* hashCode)
{
    if (hashCode == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (value)
        *hashCode = 1;
    else
        *hashCode = 0;

    return OPENDAQ_SUCCESS;
}

template <>
inline ErrCode OrdinalObjectImpl<Bool, IBoolean>::serialize(ISerializer* serializer)
{
    if (serializer == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    serializer->writeBool(value);

    return OPENDAQ_SUCCESS;
}

using BooleanImpl = OrdinalObjectImpl<Bool, IBoolean>;

END_NAMESPACE_OPENDAQ
