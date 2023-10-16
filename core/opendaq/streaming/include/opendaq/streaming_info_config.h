/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/streaming_info.h>
#include <opendaq/context.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IStreamingInfo, GenericStreamingInfoPtr)]
 */

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_info Streaming info
 * @{
 */

/*!
 * @brief Configuration component of Streaming Info objects. Contains setter methods that allow
 * for configuration of object properties.
 */
DECLARE_OPENDAQ_INTERFACE(IStreamingInfoConfig, IStreamingInfo)
{
    /*!
     * @brief Sets the network address of the device. Usually address is represented with IPv4 address.
     * @param address The network address of the device.
     */
    virtual ErrCode INTERFACE_FUNC setPrimaryAddress(IString* address) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    StreamingInfoConfig, IStreamingInfoConfig,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
