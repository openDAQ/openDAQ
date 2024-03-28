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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IDeviceInfo, GenericDeviceInfoPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device_info Device info private
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IDeviceInfoPrivate, IBaseObject)
{

    /*!
     * @brief Adds a protocol to the list of supported capabilities.
     * @param serverCapability The supported protocol to add.
     */
    virtual ErrCode INTERFACE_FUNC addServerCapability(IServerCapability* serverCapability) = 0;

    /*!
     * @brief Removes a protocol from the list of supported capabilities.
     * @param protocolId The ID of the protocol to remove.
     */
    virtual ErrCode INTERFACE_FUNC removeServerCapability(IString* protocolId) = 0;

    /*!
     * @brief Removes all server streaming capabilities from the list of supported capabilities.
     */
    virtual ErrCode INTERFACE_FUNC clearServerStreamingCapabilities() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
