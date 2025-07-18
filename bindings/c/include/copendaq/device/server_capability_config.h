//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:11.
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

    typedef struct daqServerCapabilityConfig daqServerCapabilityConfig;
    typedef struct daqString daqString;
    typedef struct daqInteger daqInteger;
    typedef struct daqAddressInfo daqAddressInfo;

    EXPORTED extern const daqIntfID DAQ_SERVER_CAPABILITY_CONFIG_INTF_ID;

    daqErrCode EXPORTED daqServerCapabilityConfig_setConnectionString(daqServerCapabilityConfig* self, daqString* connectionString);
    daqErrCode EXPORTED daqServerCapabilityConfig_addConnectionString(daqServerCapabilityConfig* self, daqString* connectionString);
    daqErrCode EXPORTED daqServerCapabilityConfig_setProtocolId(daqServerCapabilityConfig* self, daqString* protocolId);
    daqErrCode EXPORTED daqServerCapabilityConfig_setProtocolName(daqServerCapabilityConfig* self, daqString* protocolName);
    daqErrCode EXPORTED daqServerCapabilityConfig_setProtocolType(daqServerCapabilityConfig* self, daqProtocolType type);
    daqErrCode EXPORTED daqServerCapabilityConfig_setPrefix(daqServerCapabilityConfig* self, daqString* prefix);
    daqErrCode EXPORTED daqServerCapabilityConfig_setConnectionType(daqServerCapabilityConfig* self, daqString* type);
    daqErrCode EXPORTED daqServerCapabilityConfig_setCoreEventsEnabled(daqServerCapabilityConfig* self, daqBool enabled);
    daqErrCode EXPORTED daqServerCapabilityConfig_addAddress(daqServerCapabilityConfig* self, daqString* address);
    daqErrCode EXPORTED daqServerCapabilityConfig_setPort(daqServerCapabilityConfig* self, daqInteger* port);
    daqErrCode EXPORTED daqServerCapabilityConfig_addAddressInfo(daqServerCapabilityConfig* self, daqAddressInfo* addressInfo);
    daqErrCode EXPORTED daqServerCapabilityConfig_setProtocolVersion(daqServerCapabilityConfig* self, daqString* version);
    daqErrCode EXPORTED daqServerCapabilityConfig_createServerCapability(daqServerCapabilityConfig** obj, daqString* protocolId, daqString* protocolName, daqProtocolType protocolType);

#ifdef __cplusplus
}
#endif
