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
#include <opendaq/device.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IDevice, GenericDevicePtr, "<opendaq/device_ptr.h>")]
 * [templated(defaultAliasName: MirroredDevicePtr)]
 * [interfaceSmartPtr(IMirroredDevice, GenericMirroredDevicePtr)]
 */

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_device Mirrored Device
 * @{
 */

/*!
 * @brief Represents an openDAQ mirrored client device.
 * Allows accessing streaming data sources associated with the device.
 */
DECLARE_OPENDAQ_INTERFACE(IMirroredDevice, IDevice)
{
    // [elementType(streamingSources, IStreaming)]
    /*!
     * @brief Gets a list of streaming objects representing all streaming sources of the device.
     * @param[out] streamingSources The list of streaming objects.
     */
    virtual ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingSources) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
