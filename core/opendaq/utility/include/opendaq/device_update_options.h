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
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_device_options DeviceOptions
 * @{
 */

/*!
 * @brief Defines how a given device should be updated.
 */
enum class DeviceUpdateMode : EnumType
{
    Load = 0,    ///< The device is updated and added if missing.
    UpdateOnly,  ///< The device is updated, but not added if missing.
    Skip,        ///< The device is not updated. Does neither add nor remove the device.
    Remove,      ///< Removes the device from the tree, if present.
    Remap        ///< Same as load, but removes old device (if present) and uses the new manufacturer, serial number, and connection string to add a new one and update.
};

/*!
 * @brief Allows for specifying how a device and its subdevices are to be updated when loading a configuration.
 *
 * The device options object can be created using a serialized openDAQ JSON setup. The options are structured
 * hierarchically, so that options for subdevices are stored in the parent device options. The following metadata
 * is read from the setup: local ID, manufacturer, serial number, and connection string.
 *
 * For each individual device in the setup, the update mode can be specified. The update mode defines how the device is updated when loading:
 *  - Load: Updates the device. Adds the device if missing.
 *  - UpdateOnly: The device is updated, but not added if missing.
 *  - Skip: The device is not updated. Does neither add nor remove the device.
 *  - Remove: Removes the device from the tree, if present.
 *  - Remap: Same as load, but removes old device (if present) and uses the new manufacturer, serial number, and connection string to add a new one and update.
 *
 * @subsection opendaq_device_options_remapping Remapping
 * 
 * When the update mode is set to Remap, the device is removed and added again with the new manufacturer, serial number, and connection
 * string. This can be used when swapping out a device with a different one of the same make/model. This option will try and load the same
 * settings used to configure the original device to the new one.
 *
 * When loading, the new manufacturer + serial number combination will have priority over the new connection string. In case a device with
 * the new manufacturer + serial number combination is not found, the new connection string will be used as fallback.
 * 
 * If the remapping fails (e.g. due to missing device with the new manufacturer + serial number combination and invalid connection string),
 * the original device will be removed (if present) and no new device will be added.
 *
 * Signal->Input port connections will be preserved when remapping, assuming the new device has the same signal and input port structure and IDs.
 */
DECLARE_OPENDAQ_INTERFACE(IDeviceUpdateOptions, IBaseObject)
{
    /*!
     * @brief Gets the local ID of the device. The local ID is obtained from the JSON key for the device entry.
     * @param localId The local ID of the device.
     *
     * The root device might not have the local ID configured. In this case the local ID will be set to "Root".
     */
    virtual ErrCode INTERFACE_FUNC getLocalId(IString** localId) = 0;
    /*!
     * @brief Gets the manufacturer of the device, as written in the setup string.
     * @param manufacturer The manufacturer of the device.
     */
    virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;
    /*!    
     * @brief Gets the serial number of the device, as written in the setup string.
     * @param serialNumber The serial number of the device.
     */
    virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;
    /*!    
     * @brief Gets the connection string of the device, as written in the setup string.
     * @param connectionString The connection string of the device.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;
        
    /*!    
     * @brief Sets the new manufacturer of the device to be used for remapping.
     * @param manufacturer The new manufacturer of the device to be used for remapping.
     */
    virtual ErrCode INTERFACE_FUNC setNewManufacturer(IString* manufacturer) = 0;
    /*!    
     * @brief Gets the new manufacturer of the device to be used for remapping.
     * @param manufacturer The new manufacturer of the device to be used for remapping.
     */
    virtual ErrCode INTERFACE_FUNC getNewManufacturer(IString** manufacturer) = 0;
    /*!    
     * @brief Sets the new serial number of the device to be used for remapping.
     * @param serialNumber The new serial number of the device to be used for remapping.
     */
    virtual ErrCode INTERFACE_FUNC setNewSerialNumber(IString* serialNumber) = 0;
    /*!    
     * @brief Gets the new serial number of the device to be used for remapping.
     * @param serialNumber The new serial number of the device to be used for remapping.
     */
    virtual ErrCode INTERFACE_FUNC getNewSerialNumber(IString** serialNumber) = 0;
    /*!    
     * @brief Sets the new connection string of the device to be used for remapping.
     * @param connectionString The new connection string of the device to be used for remapping.
     *
     * The manufacturer and serial number combination has priority over the connection string when remapping.
     */
    virtual ErrCode INTERFACE_FUNC setNewConnectionString(IString* connectionString) = 0;
    /*!    
     * @brief Gets the new connection string of the device to be used for remapping.
     * @param connectionString The new connection string of the device to be used for remapping.
     *
     * The manufacturer and serial number combination has priority over the connection string when remapping.
     */
    virtual ErrCode INTERFACE_FUNC getNewConnectionString(IString** connectionString) = 0;

    /*!
     * @brief Gets the update mode for the device.
     * @param mode The update mode for the device.
     */
    virtual ErrCode INTERFACE_FUNC getUpdateMode(DeviceUpdateMode* mode) = 0;
    /*!
     * @brief Sets the update mode for the device.
     * @param mode The update mode for the device.
     */
    virtual ErrCode INTERFACE_FUNC setUpdateMode(DeviceUpdateMode mode) = 0;

    // [elementType(childDeviceOptions, IDeviceUpdateOptions)]
    /*!
     * @brief Gets the list of child device options for the device. These are used to configure subdevices of the current device.
     * @param childDeviceOptions The list of child device options for the device.
     */
    virtual ErrCode INTERFACE_FUNC getChildDeviceOptions(IList** childDeviceOptions) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceUpdateOptions, IString*, setupString);

END_NAMESPACE_OPENDAQ
