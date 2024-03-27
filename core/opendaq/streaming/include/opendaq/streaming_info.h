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
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(defaultAliasName: StreamingInfoPtr)]
 * [interfaceSmartPtr(IStreamingInfo, GenericStreamingInfoPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_info Streaming info
 * @{
 */

/*!
 * @brief Contains information about a streaming service available for openDAQ device.
 * Provides getter methods for commonly used properties.
 *
 * Also allows for custom properties of any type to be added.
 */
DECLARE_OPENDAQ_INTERFACE(IStreamingInfo, IPropertyObject)
{
    /*!
     * @brief Gets the network address of the device. Usually address is represented with IPv4 address.
     * @param[out] address The network address of the device.
     */
    virtual ErrCode INTERFACE_FUNC getPrimaryAddress(IString** address) = 0;

    /*!
     * @brief Gets the unique streaming protocol type ID string.
     * @param[out] protocolId The unique ID of a streaming protocol type.
     */
    virtual ErrCode INTERFACE_FUNC getProtocolId(IString** protocolId) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
