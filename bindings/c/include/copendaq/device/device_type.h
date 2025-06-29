//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:07.
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

    typedef struct daqDeviceType daqDeviceType;
    typedef struct daqString daqString;
    typedef struct daqPropertyObject daqPropertyObject;

    EXPORTED extern const daqIntfID DAQ_DEVICE_TYPE_INTF_ID;

    daqErrCode EXPORTED daqDeviceType_getConnectionStringPrefix(daqDeviceType* self, daqString** prefix);
    daqErrCode EXPORTED daqDeviceType_createDeviceType(daqDeviceType** obj, daqString* id, daqString* name, daqString* description, daqPropertyObject* defaultConfig, daqString* prefix);

#ifdef __cplusplus
}
#endif
