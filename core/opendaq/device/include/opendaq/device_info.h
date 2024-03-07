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
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/device_type.h>
#include <opendaq/server_capability.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(defaultAliasName: DeviceInfoPtr)]
 * [interfaceSmartPtr(IDeviceInfo, GenericDeviceInfoPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [interfaceSmartPtr(IDeviceType, GenericDeviceTypePtr)]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device_info Device info
 * @{
 */

/*!
 * @brief Contains standard information about an openDAQ device and device type. The Device Info object is a
 * Property Object, allowing for custom String, Int, Bool, or Float-type properties to be added.
 *
 * The getter methods represent a standardized set of Device properties according to the
 * OPC UA: Devices standard. Any additional String, Int, Bool, or Float-type properties can added, using the
 * appropriate Property Object "add property" method. Any other types of properties are invalid.
 * Although Integer-type properties are valid additions, Selection properties cannot be added to
 * Device Info.
 *
 * As the Device Info object adheres to the OPC UA: Devices standard, it behaves differently than
 * standard Property Objects. No metadata except the Value Type and Default Value are published
 * via OPC UA, and this only said Property metadata is visible to any clients.
 *
 * All fields - default (eg. platform, manufacturer...) and custom are represented as either:
 *
 * - String-type properties
 * - Integer-type properties
 * - Bool-type properties
 * - Float type properties
 *
 * As such, listing all properties via Property Object methods, will return both the
 * names of the default and custom properties. All default properties are initialized to an empty
 * string with the exception of revisionCounter and Position that are integer properties and are
 * thus initialized to '0'. The names of the properties are written in camelCase - for
 * example "systemUuid", "parentMacAddress", "manufacturerUri".
 *
 * If the DeviceInfo object is obtained from a device, or when listing available devices, the
 * object is frozen (immutable). As such, Property Object setter methods cannot be used
 * and will fail when called.
 */
DECLARE_OPENDAQ_INTERFACE(IDeviceInfo, IPropertyObject)
{
    /*!
     * @brief Gets the name of the device
     * @param[out] name The name of the device.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the string representation of a connection address used to connect to the device.
     * @param[out] connectionString The string used to connect to the device.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

    // [templateType(deviceType, IDeviceType)]
    /*!
     * @brief Gets a device type as an object providing type id, name, short description and
     * default device configuration.
     * By using default config object as a starting point, users can easily modify the preset
     * properties to tailor the configuration of the client device accordingly.
     * @param[out] deviceType The device type object
     */
    virtual ErrCode INTERFACE_FUNC getDeviceType(IDeviceType** deviceType) = 0;

    /*!
     * @brief Gets the company that manufactured the device
     * @param[out] manufacturer The manufacturer of the device.
     */
    virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;

    /*!
     * @brief Gets the unique identifier of the company that manufactured the device
     * This identifier should be a fully qualified domain name;
     * however, it may be a GUID or similar construct that ensures global uniqueness.
     * @param[out] manufacturerUri The manufacturer uri of the device.
     */
    virtual ErrCode INTERFACE_FUNC getManufacturerUri(IString** manufacturerUri) = 0;

    /*!
     * @brief Gets the model of the device
     * @param[out] model The model of the device.
     */
    virtual ErrCode INTERFACE_FUNC getModel(IString** model) = 0;

    /*!
     * @brief Gets the unique combination of numbers and letters used to identify the device.
     * @param[out] productCode The product code of the device.
     */
    virtual ErrCode INTERFACE_FUNC getProductCode(IString** productCode) = 0;

    /*!
     * @brief Gets the revision level of the device.
     * @param[out] deviceRevision The device revision level.
     */
    virtual ErrCode INTERFACE_FUNC getDeviceRevision(IString** deviceRevision) = 0;

    /*!
     * @brief Gets the revision level of the hardware.
     * @param[out] hardwareRevision The hardware revision of the device.
     */
    virtual ErrCode INTERFACE_FUNC getHardwareRevision(IString** hardwareRevision) = 0;

    /*!
     * @brief Gets the revision level of the software component.
     * @param[out] softwareRevision The software revision of the device.
     */
    virtual ErrCode INTERFACE_FUNC getSoftwareRevision(IString** softwareRevision) = 0;

    /*!
     * @brief Gets the address of the user manual.
     * It may be a pathname in the file system or a URL (Web address)
     * @param[out] deviceManual The manual of the device.
     */
    virtual ErrCode INTERFACE_FUNC getDeviceManual(IString** deviceManual) = 0;

    /*!
     * @brief Gets the purpose of the device. For example "TestMeasurementDevice".
     * @param[out] deviceClass The class of the device.
     */
    virtual ErrCode INTERFACE_FUNC getDeviceClass(IString** deviceClass) = 0;

    /*!
     * @brief Gets the unique production number provided by the manufacturer
     * @param[out] serialNumber The serial number of the device.
     */
    virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;

    /*!
     * @brief Gets the globally unique resource identifier provided by the manufacturer.
     * The recommended syntax of the ProductInstanceUri is: <ManufacturerUri>/<any string>
     * where <any string> is unique among all instances using the same ManufacturerUri.
     * @param[out] productInstanceUri The product instance uri of the device.
     */
    virtual ErrCode INTERFACE_FUNC getProductInstanceUri(IString** productInstanceUri) = 0;

    /*!
     * @brief Gets the incremental counter indicating the number of times the configuration
     * data has been modified.
     * @param[out] revisionCounter The revision counter of the device.
     */
    virtual ErrCode INTERFACE_FUNC getRevisionCounter(Int* revisionCounter) = 0;

    /*!
     * @brief Gets the asset ID of the device. Represents a user writable alphanumeric character
     * sequence uniquely identifying a component.
     * @param[out] id The asset ID of the device.
     *
     * The ID is provided by the integrator or user of the device. It contains typically an identifier
     * in a branch, use case or user specific naming scheme. This could be for example a reference to
     * an electric scheme. The ID must be a string representation of an Int32 number.
     */
    virtual ErrCode INTERFACE_FUNC getAssetId(IString** id) = 0;

    /*!
     * @brief Gets the Mac address of the device.
     * @param[out] macAddress The Mac address.
     */
    virtual ErrCode INTERFACE_FUNC getMacAddress(IString** macAddress) = 0;

    /*!
     * @brief Gets the Mac address of the device's parent.
     * @param[out] macAddress The parent's Mac address.
     */
    virtual ErrCode INTERFACE_FUNC getParentMacAddress(IString** macAddress) = 0;

    /*!
     * @brief Gets the platform of the device. The platform specifies whether real hardware
     * is used or if the device is simulated.
     * @param[out] platform The platform of the device.
     */
    virtual ErrCode INTERFACE_FUNC getPlatform(IString** platform) = 0;

    /*!
     * @brief Gets the position of the device. The position specifies the position within a
     * given system. For example in which slot or slice the device is in.
     * @param[out] position The position of the device.
     *
     * The Position should be a positive integer in the range supported by the UInt16 data type.
     */
    virtual ErrCode INTERFACE_FUNC getPosition(Int* position) = 0;

    /*!
     * @brief Gets the system type. The system type can, for example, be LayeredSystem,
     * StandaloneSystem, or RackSystem.
     * @param[out] type The system type of the device.
     */
    virtual ErrCode INTERFACE_FUNC getSystemType(IString** type) = 0;

    /*!
     * @brief Gets the system UUID that represents a unique ID of a system. All devices in a system
     * share this UUID.
     * @param[out] uuid The unique ID of a system.
     */
    virtual ErrCode INTERFACE_FUNC getSystemUuid(IString** uuid) = 0;

    // [templateType(customInfoNames, IString)]
    /*!
     * @brief Gets the list of property names that are not in the default set of Device info properties.
     * Default properties are all info properties that have a corresponding getter method.
     * @param[out] customInfoNames The list of names of custom properties.
     */
    virtual ErrCode INTERFACE_FUNC getCustomInfoPropertyNames(IList** customInfoNames) = 0;

    /*!
     * @brief Gets the version of the SDK used to build said device. Can be empty if the device does not use
     * the SDK as its firmware/is implemented at a protocol-level.
     * @param[out] version The SDK version.
     */
    virtual ErrCode INTERFACE_FUNC getSdkVersion(IString** version) = 0;

    // [elementType(deviceCapabilities, IServerCapability)]
    /*!
     * @brief Gets the list of device supported protocols
     * @param[out] deviceCapabilities The list of device supported protocols
     */
    virtual ErrCode INTERFACE_FUNC getDeviceCapabilities(IList** deviceCapabilities) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
