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
#include <opendaq/streaming.h>
#include <opendaq/mirrored_device.h>
#include <opendaq/mirrored_input_port_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IStreaming, GenericStreamingPtr, "<opendaq/streaming_ptr.h>")]
 * [templated(defaultAliasName: StreamingToDevicePtr)]
 * [interfaceSmartPtr(IStreamingToDevice, GenericStreamingToDevicePtr)]
 */

DECLARE_OPENDAQ_INTERFACE(IStreamingToDevice, IStreaming)
{
    // [elementType(inputPorts, IMirroredInputPortConfig)]
    virtual ErrCode INTERFACE_FUNC addInputPorts(IList* inputPorts) = 0;

    // [elementType(inputPorts, IMirroredInputPortConfig)]
    virtual ErrCode INTERFACE_FUNC removeInputPorts(IList* inputPorts) = 0;

    virtual ErrCode INTERFACE_FUNC removeAllInputPorts() = 0;

    virtual ErrCode INTERFACE_FUNC getOwnerDevice(IMirroredDevice** device) const = 0;
    virtual ErrCode INTERFACE_FUNC getProtocolId(IString** protocolId) const = 0;
};

END_NAMESPACE_OPENDAQ



