/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include "opcuatms_client/objects/tms_client_context.h"
#include "opcuatms_client/objects/tms_client_device_impl.h"
#include <opendaq/device_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

inline DevicePtr TmsClientDevice(const ContextPtr& context,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const daq::opcua::tms::TmsClientContextPtr& clientContext,
                                 const opcua::OpcUaNodeId& nodeId,
                                 const FunctionPtr& createStreamingCallback)
{
    DevicePtr obj(createWithImplementation<IDevice, TmsClientDeviceImpl>(context, parent, localId, clientContext, nodeId, createStreamingCallback, false));
    return obj;
}

inline DevicePtr TmsClientRootDevice(const ContextPtr& context,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     const daq::opcua::tms::TmsClientContextPtr& clientContext,
                                     const opcua::OpcUaNodeId& nodeId,
                                     const FunctionPtr& createStreamingCallback)
{
    DevicePtr obj(createWithImplementation<IDevice, TmsClientDeviceImpl>(context, parent, localId, clientContext, nodeId, createStreamingCallback, true));
    return obj;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
