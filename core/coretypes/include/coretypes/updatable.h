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
#include <coretypes/baseobject.h>
#include <coretypes/serialized_object.h>
#include <coretypes/serializer.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_utility
 * @defgroup types_updatable Updatable
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IUpdatable, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC update(ISerializedObject* update, IBaseObject* config) = 0;
    virtual ErrCode INTERFACE_FUNC serializeForUpdate(ISerializer* serializer) = 0;
    virtual ErrCode INTERFACE_FUNC updateEnded(IBaseObject* context) = 0;
    virtual ErrCode INTERFACE_FUNC updateInternal(ISerializedObject* update, IBaseObject* context) = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
