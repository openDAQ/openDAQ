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
 * @addtogroup opendaq_device_info Device info
 * @{
 */

/*!
 * @brief Configuration component of Device info objects. Contains setter methods that
 * are available until the object is frozen.
 *
 * Device info config contains functions that allow for the configuration of Device info objects.
 * The implementation of `config` also implements the `freeze` function that freezes the object making it
 * immutable. Once frozen, the setter methods fail as the object can no longer be modified.
 */
DECLARE_OPENDAQ_INTERFACE(IDeviceInfoPrivate, IBaseObject)
{

    /*!
     * @brief Add protocol to the list of support capabilities
     * @param serverCapability The supported protocol.
     */
    virtual ErrCode INTERFACE_FUNC addServerCapability(IServerCapability* serverCapability) = 0;

    /*!
     * @brief Remove protocol from the list of support capabilities
     * @param serverCapability The supported protocol.
     */
    virtual ErrCode INTERFACE_FUNC removeServerCapability(IString* protocolId) = 0;

    /*!
     * @brief Remove server streaming capabilities
     */
    virtual ErrCode INTERFACE_FUNC clearServerStreamingCapabilities() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
