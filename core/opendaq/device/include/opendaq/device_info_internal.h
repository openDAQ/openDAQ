/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

    // [templateType(changeableProperties, IString)]
    /*!
     * @brief Sets the list of properties that can be changed.
     * @param changeableProperties The list of property names that can be changed.
     * 
     * This method sets the list of changeable properties. However, it does not immediately mark the properties as editable. Applying the list happening with setting the owner.
     * After applying the list:
     * - Properties which are not existing in the owner device will not be marked as editable.
     * - All changeable properties that do not exist in the device info but exist in the device, will be added from clone property of the device.
     * 
     * If the owner device is destroyed or replaced:
     * - The changeable properties will reference the new owner device, provided it exists and has the specified property.
     * - If the new owner does not exist or lacks the property, the property will default to the one in the device info.
     * 
     * Note: Setting the list of changeable properties takes effect only before the owner of the device info is set. If the list is updated after the owner is set, the new list will be ignored.
     */
    virtual ErrCode INTERFACE_FUNC setChangeableProperties(IList* changeableProperties) = 0;

    // [templateType(changeableProperties, IString)]
    /*!
     * @brief Retrieves the list of properties that can potentially be marked as changeable.
     * @param changeableProperties A list that will be populated with the current changeable properties.
     * 
     * This method returns the list of properties that were set as potentially changeable using the `setChangeableProperties` method. However, the actual properties might not yet be marked as changeable due to various conditions:
     * - A property from the list may not exist in the owner device or may not yet be editable.
     * 
     * After the list of changeable properties has been applied via `setOwner`, this method will always return the applied list. The list reflects the configuration but does not guarantee that all listed properties are currently editable or marked as changeable in the owner device.
     */
    virtual ErrCode INTERFACE_FUNC getChangeableProperties(IList** changeableProperties) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
