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
#include <opendaq/device_update_options.h>
#include <rapidjson/document.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/device_update_options_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

enum class NodeType : EnumType
{
    Instance = 0,
    Folder,
    Device,
    Unknown = 99
};

class DeviceUpdateOptionsImpl : public ImplementationOf<IDeviceUpdateOptions, ISerializable>
{
public:
    DeviceUpdateOptionsImpl();
    DeviceUpdateOptionsImpl(const StringPtr& setupString);
    
    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;
    ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;

    ErrCode INTERFACE_FUNC setNewManufacturer(IString* manufacturer) override;
    ErrCode INTERFACE_FUNC getNewManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC setNewSerialNumber(IString* serialNumber) override;
    ErrCode INTERFACE_FUNC getNewSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC setNewConnectionString(IString* connectionString) override;
    ErrCode INTERFACE_FUNC getNewConnectionString(IString** connectionString) override;

    ErrCode INTERFACE_FUNC getUpdateMode(DeviceUpdateMode* mode) override;
    ErrCode INTERFACE_FUNC setUpdateMode(DeviceUpdateMode mode) override;
    ErrCode INTERFACE_FUNC getChildDeviceOptions(IList** childDeviceOptions) override;
    
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;
    
    // ISerializable

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* /*factoryCallback*/, IBaseObject** obj);

    bool readDevice(const StringPtr& localId, const rapidjson::Value& document);
    

    static NodeType getNodeType(const rapidjson::Value& value);

    bool read(const StringPtr& localId, const rapidjson::Value& document, NodeType nodeType);
    bool readFolder(const rapidjson::Value& document);
    
    bool isRoot;
    StringPtr localId;
    StringPtr manufacturer;
    StringPtr serialNumber;
    StringPtr connectionString;

    StringPtr newManufacturer;
    StringPtr newSerialNumber;
    StringPtr newConnectionString;

    DeviceUpdateMode mode;
    ListPtr<IDeviceUpdateOptions> children;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DeviceUpdateOptionsImpl)

END_NAMESPACE_OPENDAQ
