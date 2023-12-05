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
#include <opendaq/device_info_config.h>
#include <coretypes/freezable.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/device_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDeviceInfoConfig, typename... Interfaces>
class DeviceInfoConfigImpl;

using DeviceInfoConfigBase = DeviceInfoConfigImpl<>;

template <typename TInterface, typename... Interfaces>
class DeviceInfoConfigImpl : public GenericPropertyObjectImpl<TInterface, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<TInterface, Interfaces...>;

    explicit DeviceInfoConfigImpl(const StringPtr& name, const StringPtr& connectionString);

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getDeviceType(IDeviceType** deviceType) override;
    ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC getManufacturerUri(IString** manufacturerUri) override;
    ErrCode INTERFACE_FUNC getModel(IString** model) override;
    ErrCode INTERFACE_FUNC getProductCode(IString** productCode) override;
    ErrCode INTERFACE_FUNC getDeviceRevision(IString** deviceRevision) override;
    ErrCode INTERFACE_FUNC getHardwareRevision(IString** hardwareRevision) override;
    ErrCode INTERFACE_FUNC getSoftwareRevision(IString** softwareRevision) override;
    ErrCode INTERFACE_FUNC getDeviceManual(IString** deviceManual) override;
    ErrCode INTERFACE_FUNC getDeviceClass(IString** deviceClass) override;
    ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC getProductInstanceUri(IString** productInstanceUri) override;
    ErrCode INTERFACE_FUNC getRevisionCounter(Int* revisionCounter) override;
    ErrCode INTERFACE_FUNC getAssetId(IString** id) override;
    ErrCode INTERFACE_FUNC getMacAddress(IString** macAddress) override;
    ErrCode INTERFACE_FUNC getParentMacAddress(IString** macAddress) override;
    ErrCode INTERFACE_FUNC getPlatform(IString** platform) override;
    ErrCode INTERFACE_FUNC getPosition(Int* position) override;
    ErrCode INTERFACE_FUNC getSystemType(IString** type) override;
    ErrCode INTERFACE_FUNC getSystemUuid(IString** uuid) override;
    ErrCode INTERFACE_FUNC getCustomInfoPropertyNames(IList** customInfoNames) override;

    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) override;
    ErrCode INTERFACE_FUNC setDeviceType(IDeviceType* deviceType) override;
    ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) override;
    ErrCode INTERFACE_FUNC setManufacturerUri(IString* manufacturerUri) override;
    ErrCode INTERFACE_FUNC setModel(IString* model) override;
    ErrCode INTERFACE_FUNC setProductCode(IString* productCode) override;
    ErrCode INTERFACE_FUNC setDeviceRevision(IString* deviceRevision) override;
    ErrCode INTERFACE_FUNC setHardwareRevision(IString* hardwareRevision) override;
    ErrCode INTERFACE_FUNC setSoftwareRevision(IString* softwareRevision) override;
    ErrCode INTERFACE_FUNC setDeviceManual(IString* deviceManual) override;
    ErrCode INTERFACE_FUNC setDeviceClass(IString* deviceClass) override;
    ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) override;
    ErrCode INTERFACE_FUNC setProductInstanceUri(IString* productInstanceUri) override;
    ErrCode INTERFACE_FUNC setRevisionCounter(Int revisionCounter) override;
    ErrCode INTERFACE_FUNC setAssetId(IString* id) override;;
    ErrCode INTERFACE_FUNC setMacAddress(IString* macAddress) override;
    ErrCode INTERFACE_FUNC setParentMacAddress(IString* macAddress) override;
    ErrCode INTERFACE_FUNC setPlatform(IString* platform) override;
    ErrCode INTERFACE_FUNC setPosition(Int position) override;
    ErrCode INTERFACE_FUNC setSystemType(IString* type) override;
    ErrCode INTERFACE_FUNC setSystemUuid(IString* uuid) override;

    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;

private:
    ErrCode createAndSetDefaultStringProperty(const StringPtr& name, const BaseObjectPtr& value);
    ErrCode createAndSetStringProperty(const StringPtr& name, const StringPtr& value);
    ErrCode createAndSetDefaultIntProperty(const StringPtr& name, const BaseObjectPtr& value);
    ErrCode createAndSetIntProperty(const StringPtr& name, const IntegerPtr& value);
    StringPtr getStringProperty(const StringPtr& name);
    Int getIntProperty(const StringPtr& name);

    std::unordered_set<std::string> defaultPropertyNames;
    DeviceTypePtr deviceType;
};

END_NAMESPACE_OPENDAQ
