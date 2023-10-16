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
#include <coretypes/serializer.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_serialization
 * @defgroup types_serializable Serializable
 * @{
 */

static constexpr IntfID SerializableGuid = { 0xF2A26E1A, 0x0735, 0x5758, { { 0x88, 0xE7, 0xF4, 0x1B, 0xCB, 0x9E, 0x2E, 0xDC } } };

DECLARE_OPENDAQ_INTERFACE_EX(ISerializable, IBaseObject)
{
    DEFINE_EXTERNAL_INTFID(SerializableGuid)

    virtual ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) = 0;
    virtual ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
