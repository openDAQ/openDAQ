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
#include <opendaq/device_info.h>

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
DECLARE_OPENDAQ_INTERFACE(IDeviceInfoConfig, IDeviceInfo)
{
    /*!
     * @brief Sets the name of the device
     * @param name The name of the device.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Sets the string representation of a connection address used to connect to the device.
     * @param connectionString The string used to connect to the device.
     */
    virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

    /*!
     * @brief Sets a device type as an object providing type id, name, short description and
     * default device configuration.
     * @param[out] deviceType The device type object
     */
    virtual ErrCode INTERFACE_FUNC setDeviceType(IDeviceType* deviceType) = 0;

    /*!
     * @brief Sets the company that manufactured the device
     * @param manufacturer The manufacturer of the device.
     */
    virtual ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) = 0;

    /*!
     * @brief Sets the unique identifier of the company that manufactured the device.
     * This identifier should be a fully qualified domain name;
     * however, it may be a GUID or similar construct that ensures global uniqueness.
     * @param manufacturerUri The manufacturer uri of the device.
     */
    virtual ErrCode INTERFACE_FUNC setManufacturerUri(IString* manufacturerUri) = 0;

    /*!
     * @brief Sets the model of the device
     * @param model The model of the device.
     */
    virtual ErrCode INTERFACE_FUNC setModel(IString* model) = 0;

    /*!
     * @brief Sets the unique combination of numbers and letters used to identify the device.
     * @param productCode The product code of the device.
     */
    virtual ErrCode INTERFACE_FUNC setProductCode(IString* productCode) = 0;

    /*!
     * @brief Sets the revision level of the device.
     * @param deviceRevision The device revision level.
     */
    virtual ErrCode INTERFACE_FUNC setDeviceRevision(IString* deviceRevision) = 0;

    /*!
     * @brief Sets the revision level of the hardware
     * @param hardwareRevision The hardware revision of the device.
     */
    virtual ErrCode INTERFACE_FUNC setHardwareRevision(IString* hardwareRevision) = 0;

    /*!
     * @brief sets the revision level of the software component
     * @param softwareRevision The software revision of the device.
     */
    virtual ErrCode INTERFACE_FUNC setSoftwareRevision(IString* softwareRevision) = 0;

    /*!
     * @brief Sets the address of the user manual.
     * It may be a pathname in the file system or a URL (Web address)
     * @param deviceManual The manual of the device.
     */
    virtual ErrCode INTERFACE_FUNC setDeviceManual(IString* deviceManual) = 0;

    /*!
     * @brief Sets the purpose of the device. For example "TestMeasurementDevice".
     * @param deviceClass The class of the device.
     */
    virtual ErrCode INTERFACE_FUNC setDeviceClass(IString* deviceClass) = 0;

    /*!
     * @brief Sets the serial number of the device
     * @param serialNumber The serial number of the device.
     */
    virtual ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) = 0;

    /*!
     * @brief Sets the globally unique resource identifier provided by the manufacturer.
     * The recommended syntax of the ProductInstanceUri is: <ManufacturerUri>/<any string>
     * where <any string> is unique among all instances using the same ManufacturerUri.
     * @param productInstanceUri The product instance uri of the device.
     */
    virtual ErrCode INTERFACE_FUNC setProductInstanceUri(IString* productInstanceUri) = 0;

    /*!
     * @brief Sets the incremental counter indicating the number of times the configuration
     * data has been modified.
     * @param revisionCounter The revision counter of the device.
     */
    virtual ErrCode INTERFACE_FUNC setRevisionCounter(Int revisionCounter) = 0;

    /*!
     * @brief Sets the asset ID of the device. Represents a user writable alphanumeric character
     * sequence uniquely identifying a component.
     * @param id The asset ID of the device.
     *
     * The ID is provided by the integrator or user of the device. It contains typically an identifier
     * in a branch, use case or user specific naming scheme. This could be for example a reference to
     * an electric scheme. 
     */
    virtual ErrCode INTERFACE_FUNC setAssetId(IString* id) = 0;

    /*!
     * @brief Sets the Mac address of the device.
     * @param macAddress The Mac address.
     */
    virtual ErrCode INTERFACE_FUNC setMacAddress(IString* macAddress) = 0;

    /*!
     * @brief Sets the Mac address of the device's parent.
     * @param macAddress The parent's Mac address.
     */
    virtual ErrCode INTERFACE_FUNC setParentMacAddress(IString* macAddress) = 0;

    /*!
     * @brief Sets the platform of the device. The platform specifies whether real hardware
     * is used or if the device is simulated.
     * @param platform The platform of the device.
     */
    virtual ErrCode INTERFACE_FUNC setPlatform(IString* platform) = 0;

    /*!
     * @brief Sets the position of the device. The position specifies the position within a
     * given system. For example in which slot or slice the device is in.
     * @param position The platform of the device.
     */
    virtual ErrCode INTERFACE_FUNC setPosition(Int position) = 0;

    /*!
     * @brief Sets the system type. The system type can, for example, be LayeredSystem,
     * StandaloneSystem, or RackSystem.
     * @param type The system type of the device.
     */
    virtual ErrCode INTERFACE_FUNC setSystemType(IString* type) = 0;

    /*!
     * @brief Sets the system UUID that represents a unique ID of a system. All devices in a system
     * share this UUID.
     * @param uuid The unique ID of a system.
     */
    virtual ErrCode INTERFACE_FUNC setSystemUuid(IString* uuid) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DeviceInfoConfig, IDeviceInfoConfig, IString*, name, IString*, connectionString)

END_NAMESPACE_OPENDAQ
