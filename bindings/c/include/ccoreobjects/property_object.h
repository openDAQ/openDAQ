//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:02:11.
// </auto-generated>
//------------------------------------------------------------------------------

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

    typedef struct PropertyObject PropertyObject;
    typedef struct String String;
    typedef struct Property Property;
    typedef struct Event Event;
    typedef struct List List;
    typedef struct PermissionManager PermissionManager;
    typedef struct TypeManager TypeManager;

    EXPORTED extern const IntfID PROPERTY_OBJECT_INTF_ID;

    ErrCode EXPORTED PropertyObject_getClassName(PropertyObject* self, String** className);
    ErrCode EXPORTED PropertyObject_setPropertyValue(PropertyObject* self, String* propertyName, BaseObject* value);
    ErrCode EXPORTED PropertyObject_getPropertyValue(PropertyObject* self, String* propertyName, BaseObject** value);
    ErrCode EXPORTED PropertyObject_getPropertySelectionValue(PropertyObject* self, String* propertyName, BaseObject** value);
    ErrCode EXPORTED PropertyObject_clearPropertyValue(PropertyObject* self, String* propertyName);
    ErrCode EXPORTED PropertyObject_hasProperty(PropertyObject* self, String* propertyName, Bool* hasProperty);
    ErrCode EXPORTED PropertyObject_getProperty(PropertyObject* self, String* propertyName, Property** property);
    ErrCode EXPORTED PropertyObject_addProperty(PropertyObject* self, Property* property);
    ErrCode EXPORTED PropertyObject_removeProperty(PropertyObject* self, String* propertyName);
    ErrCode EXPORTED PropertyObject_getOnPropertyValueWrite(PropertyObject* self, String* propertyName, Event** event);
    ErrCode EXPORTED PropertyObject_getOnPropertyValueRead(PropertyObject* self, String* propertyName, Event** event);
    ErrCode EXPORTED PropertyObject_getOnAnyPropertyValueWrite(PropertyObject* self, Event** event);
    ErrCode EXPORTED PropertyObject_getOnAnyPropertyValueRead(PropertyObject* self, Event** event);
    ErrCode EXPORTED PropertyObject_getVisibleProperties(PropertyObject* self, List** properties);
    ErrCode EXPORTED PropertyObject_getAllProperties(PropertyObject* self, List** properties);
    ErrCode EXPORTED PropertyObject_setPropertyOrder(PropertyObject* self, List* orderedPropertyNames);
    ErrCode EXPORTED PropertyObject_beginUpdate(PropertyObject* self);
    ErrCode EXPORTED PropertyObject_endUpdate(PropertyObject* self);
    ErrCode EXPORTED PropertyObject_getUpdating(PropertyObject* self, Bool* updating);
    ErrCode EXPORTED PropertyObject_getOnEndUpdate(PropertyObject* self, Event** event);
    ErrCode EXPORTED PropertyObject_getPermissionManager(PropertyObject* self, PermissionManager** permissionManager);
    ErrCode EXPORTED PropertyObject_createPropertyObject(PropertyObject** obj);
    ErrCode EXPORTED PropertyObject_createPropertyObjectWithClassAndManager(PropertyObject** obj, TypeManager* manager, String* className);

#ifdef __cplusplus
}
#endif
