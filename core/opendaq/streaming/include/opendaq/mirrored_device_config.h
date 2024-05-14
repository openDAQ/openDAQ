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
#include <opendaq/mirrored_device.h>
#include <opendaq/streaming_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IMirroredDevice, GenericMirroredDevicePtr)]
 * [templated(defaultAliasName: MirroredDeviceConfigPtr)]
 * [interfaceSmartPtr(IMirroredDeviceConfig, GenericMirroredDeviceConfigPtr)]
 */

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_device Mirrored Device
 * @{
 */

/*!
 * @brief Represents configuration interface for mirrored device.
 * Allows attaching and removing streaming data sources associated with the device.
 */
DECLARE_OPENDAQ_INTERFACE(IMirroredDeviceConfig, IMirroredDevice)
{
    /*!
     * @brief Adds streaming source for device.
     * @param streaming The Streaming object representing the data source.
     */
    virtual ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streamingSource) = 0;

    /*!
     * @brief Removes streaming source for device e.g. when the streaming source is no longer available.
     * @param streamingConnectionString The connection string of streaming source to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
