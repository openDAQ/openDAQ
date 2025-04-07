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

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef struct PropertyValueEventArgs PropertyValueEventArgs;
    typedef struct Property Property;

    EXPORTED extern const IntfID PROPERTY_VALUE_EVENT_ARGS_INTF_ID;

    ErrCode EXPORTED PropertyValueEventArgs_getProperty(PropertyValueEventArgs* self, Property** property);
    ErrCode EXPORTED PropertyValueEventArgs_getValue(PropertyValueEventArgs* self, BaseObject** value);
    ErrCode EXPORTED PropertyValueEventArgs_setValue(PropertyValueEventArgs* self, BaseObject* value);
    ErrCode EXPORTED PropertyValueEventArgs_getPropertyEventType(PropertyValueEventArgs* self, PropertyEventType* changeType);
    ErrCode EXPORTED PropertyValueEventArgs_getIsUpdating(PropertyValueEventArgs* self, Bool* isUpdating);
    ErrCode EXPORTED PropertyValueEventArgs_getOldValue(PropertyValueEventArgs* self, BaseObject** value);
    ErrCode EXPORTED PropertyValueEventArgs_createPropertyValueEventArgs(PropertyValueEventArgs** obj, Property* prop, BaseObject* value, BaseObject* oldValue, PropertyEventType type, Bool isUpdating);

#ifdef __cplusplus
}
#endif
