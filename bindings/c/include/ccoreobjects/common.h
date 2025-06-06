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

    typedef enum daqCoreEventId
    {
        daqCoreEventIdPropertyValueChanged = 0,
        daqCoreEventIdPropertyObjectUpdateEnd = 10,
        daqCoreEventIdPropertyAdded = 20,
        daqCoreEventIdPropertyRemoved = 30,
        daqCoreEventIdComponentAdded = 40,
        daqCoreEventIdComponentRemoved = 50,
        daqCoreEventIdSignalConnected = 60,
        daqCoreEventIdSignalDisconnected = 70,
        daqCoreEventIdDataDescriptorChanged = 80,
        daqCoreEventIdComponentUpdateEnd = 90,
        daqCoreEventIdAttributeChanged = 100,
        daqCoreEventIdTagsChanged = 110,
        daqCoreEventIdStatusChanged = 120,
        daqCoreEventIdTypeAdded = 130,
        daqCoreEventIdTypeRemoved = 140,
        daqCoreEventIdDeviceDomainChanged = 150,
        daqCoreEventIdDeviceLockStateChanged = 160,
        daqCoreEventIdConnectionStatusChanged = 170,
        daqCoreEventIdDeviceOperationModeChanged = 180,
    } daqCoreEventId;

    typedef enum daqPermission
    {
        daqPermissionNone = 0x0,    // The user has no permissions on the object.
        daqPermissionRead = 0x1,    // The user can see and read an object.
        daqPermissionWrite = 0x2,   // The user can change or write to the object.
        daqPermissionExecute = 0x4  // The user can execute an action attached to the object.
    } daqPermission;

    typedef enum daqPropertyEventType
    {
        daqPropertyEventTypeEventTypeUpdate,
        daqPropertyEventTypeEventTypeClear,
        daqPropertyEventTypeEventTypeRead
    } daqPropertyEventType;

#ifdef __cplusplus
}
#endif
