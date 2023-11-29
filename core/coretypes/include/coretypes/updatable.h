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
#include <coretypes/baseobject.h>
#include <coretypes/serialized_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_utility
 * @defgroup types_updatable Updatable
 * @{
 */

DECLARE_OPENDAQ_INTERFACE_EX(IUpdatable, IBaseObject)
{
    DEFINE_INTFID("IUpdatable")

    virtual ErrCode INTERFACE_FUNC update(ISerializedObject* update) = 0;
};

/*!
 * @}
 */

template <typename T>
using IsEnumTypeEnum = std::enable_if<std::is_enum_v<T> &&
                                      std::is_same_v<std::underlying_type_t<T>, EnumType>,
                                      int>;

template <typename T, typename IsEnumTypeEnum<T>::type = 0>
T operator|(T lhs, T rhs)
{
    return T(EnumType(lhs) | EnumType(rhs));
}

template <typename T, typename IsEnumTypeEnum<T>::type = 0>
bool operator&(T lhs, T rhs)
{
    return EnumType(lhs) & EnumType(rhs);
}

END_NAMESPACE_OPENDAQ
