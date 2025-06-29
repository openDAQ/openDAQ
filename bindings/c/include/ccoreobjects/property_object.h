//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 05.06.2025 17:27:48.
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

#include <ccommon.h>

    typedef struct daqPropertyObject daqPropertyObject;
    typedef struct daqString daqString;
    typedef struct daqProperty daqProperty;
    typedef struct daqEvent daqEvent;
    typedef struct daqList daqList;
    typedef struct daqPermissionManager daqPermissionManager;
    typedef struct daqSearchFilter daqSearchFilter;
    typedef struct daqTypeManager daqTypeManager;

    EXPORTED extern const daqIntfID DAQ_PROPERTY_OBJECT_INTF_ID;

    daqErrCode EXPORTED daqPropertyObject_getClassName(daqPropertyObject* self, daqString** className);
    daqErrCode EXPORTED daqPropertyObject_setPropertyValue(daqPropertyObject* self, daqString* propertyName, daqBaseObject* value);
    daqErrCode EXPORTED daqPropertyObject_getPropertyValue(daqPropertyObject* self, daqString* propertyName, daqBaseObject** value);
    daqErrCode EXPORTED daqPropertyObject_getPropertySelectionValue(daqPropertyObject* self, daqString* propertyName, daqBaseObject** value);
    daqErrCode EXPORTED daqPropertyObject_clearPropertyValue(daqPropertyObject* self, daqString* propertyName);
    daqErrCode EXPORTED daqPropertyObject_hasProperty(daqPropertyObject* self, daqString* propertyName, daqBool* hasProperty);
    daqErrCode EXPORTED daqPropertyObject_getProperty(daqPropertyObject* self, daqString* propertyName, daqProperty** property);
    daqErrCode EXPORTED daqPropertyObject_addProperty(daqPropertyObject* self, daqProperty* property);
    daqErrCode EXPORTED daqPropertyObject_removeProperty(daqPropertyObject* self, daqString* propertyName);
    daqErrCode EXPORTED daqPropertyObject_getOnPropertyValueWrite(daqPropertyObject* self, daqString* propertyName, daqEvent** event);
    daqErrCode EXPORTED daqPropertyObject_getOnPropertyValueRead(daqPropertyObject* self, daqString* propertyName, daqEvent** event);
    daqErrCode EXPORTED daqPropertyObject_getOnAnyPropertyValueWrite(daqPropertyObject* self, daqEvent** event);
    daqErrCode EXPORTED daqPropertyObject_getOnAnyPropertyValueRead(daqPropertyObject* self, daqEvent** event);
    daqErrCode EXPORTED daqPropertyObject_getVisibleProperties(daqPropertyObject* self, daqList** properties);
    daqErrCode EXPORTED daqPropertyObject_getAllProperties(daqPropertyObject* self, daqList** properties);
    daqErrCode EXPORTED daqPropertyObject_setPropertyOrder(daqPropertyObject* self, daqList* orderedPropertyNames);
    daqErrCode EXPORTED daqPropertyObject_beginUpdate(daqPropertyObject* self);
    daqErrCode EXPORTED daqPropertyObject_endUpdate(daqPropertyObject* self);
    daqErrCode EXPORTED daqPropertyObject_getUpdating(daqPropertyObject* self, daqBool* updating);
    daqErrCode EXPORTED daqPropertyObject_getOnEndUpdate(daqPropertyObject* self, daqEvent** event);
    daqErrCode EXPORTED daqPropertyObject_getPermissionManager(daqPropertyObject* self, daqPermissionManager** permissionManager);
    daqErrCode EXPORTED daqPropertyObject_findProperties(daqPropertyObject* self, daqList** properties, daqSearchFilter* propertyFilter, daqSearchFilter* componentFilter);
    daqErrCode EXPORTED daqPropertyObject_createPropertyObject(daqPropertyObject** obj);
    daqErrCode EXPORTED daqPropertyObject_createPropertyObjectWithClassAndManager(daqPropertyObject** obj, daqTypeManager* manager, daqString* className);

#ifdef __cplusplus
}
#endif
