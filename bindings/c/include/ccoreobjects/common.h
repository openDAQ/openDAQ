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

    enum CoreEventId
    {
        CoreEventIdPropertyValueChanged = 0,
        CoreEventIdPropertyObjectUpdateEnd = 10,
        CoreEventIdPropertyAdded = 20,
        CoreEventIdPropertyRemoved = 30,
        CoreEventIdComponentAdded = 40,
        CoreEventIdComponentRemoved = 50,
        CoreEventIdSignalConnected = 60,
        CoreEventIdSignalDisconnected = 70,
        CoreEventIdDataDescriptorChanged = 80,
        CoreEventIdComponentUpdateEnd = 90,
        CoreEventIdAttributeChanged = 100,
        CoreEventIdTagsChanged = 110,
        CoreEventIdStatusChanged = 120,
        CoreEventIdTypeAdded = 130,
        CoreEventIdTypeRemoved = 140,
        CoreEventIdDeviceDomainChanged = 150,
        CoreEventIdDeviceLockStateChanged = 160,
        CoreEventIdConnectionStatusChanged = 170,
        CoreEventIdDeviceOperationModeChanged = 180,
    };

    enum Permission
    {
        PermissionNone = 0x0,    // The user has no permissions on the object.
        PermissionRead = 0x1,    // The user can see and read an object.
        PermissionWrite = 0x2,   // The user can change or write to the object.
        PermissionExecute = 0x4  // The user can execute an action attached to the object.
    };

    enum PropertyEventType
    {
        PropertyEventTypeEventTypeUpdate,
        PropertyEventTypeEventTypeClear,
        PropertyEventTypeEventTypeRead
    };

#ifdef __cplusplus
}
#endif
