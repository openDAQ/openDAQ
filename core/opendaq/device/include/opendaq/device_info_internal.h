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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IDeviceInfo, GenericDeviceInfoPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device_info Device info internal
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IDeviceInfoInternal, IBaseObject)
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

    /*!
     * @brief Adds a network interface to the dictionary of available interfaces.
     * @param networkInterface The available interface to add.
     * @param name The name of available interface to add.
     *
     * The provided name should be unique within the device info as used as the key in the dictionary of available interfaces.
     */
    virtual ErrCode INTERFACE_FUNC addNetworkInteface(IString* name, INetworkInterface* networkInterface) = 0;

    // [arrayArg(clientNumber, 1)]
    /*!
     * @brief Registers a newly connected client or re-registers a reconnected client.
     * @param[in,out] clientNumber If provided, represents the original ordinal number of the re-registered client.
     * If unassigned or exceeding the total number of clients ever registered (including those that were later deregistered),
     * it is set to the incremented total count; otherwise, the client is registered under the specified number.
     * @param clientInfo The connected client information object.
     * @return OPENDAQ_ERR_ALREADYEXISTS if a client with the specified number already registered.
     */
    virtual ErrCode INTERFACE_FUNC addConnectedClient(SizeT* clientNumber, IConnectedClientInfo* clientInfo) = 0;

    /*!
     * @brief Unregisters a previously connected client upon disconnection.
     * @param clientNumber The number identifying the disconnected client.
     */
    virtual ErrCode INTERFACE_FUNC removeConnectedClient(SizeT clientNumber) = 0;

    // [templateType(deviceInfo, IDeviceInfo)]
    virtual ErrCode INTERFACE_FUNC mergeDeviceInfo(IDeviceInfo* deviceInfo) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
