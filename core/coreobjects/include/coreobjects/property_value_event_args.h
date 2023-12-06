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
#include <coretypes/coretypes.h>
#include <coretypes/event_args.h>
#include <coreobjects/property.h>

BEGIN_NAMESPACE_OPENDAQ

enum class PropertyEventType : EnumType
{
    Update,
    Clear,
    Read
};

/*#
 * [interfaceSmartPtr(IEventArgs, EventArgsPtr, "<coretypes/event_args_ptr.h>")]
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyValueEventArgs, IEventArgs)
{
    virtual ErrCode INTERFACE_FUNC getProperty(IProperty** property) = 0;
    virtual ErrCode INTERFACE_FUNC getValue(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setValue(IBaseObject* value) = 0;
    virtual ErrCode INTERFACE_FUNC getPropertyEventType(PropertyEventType* changeType) = 0;
    virtual ErrCode INTERFACE_FUNC getIsUpdating(Bool* isUpdating) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, PropertyValueEventArgs,
    IProperty*, prop,
    IBaseObject*, value,
    PropertyEventType, type,
    Bool,
    isUpdating
)

END_NAMESPACE_OPENDAQ
