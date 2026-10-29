/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/add_device_config_ptr.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class AddDeviceConfigImpl : public GenericPropertyObjectImpl<IPropertyObject, IAddDeviceConfig>
{
    class Partial
    {
        Partial()
        {
        }
        friend class AddDeviceConfigImpl;
    };

public:
    using Super = GenericPropertyObjectImpl<IPropertyObject, IAddDeviceConfig>;

    AddDeviceConfigImpl(const DeviceTypePtr& deviceType, const DictPtr<IString, IStreamingType>& streamingTypes, Bool scanOnAdd);
    AddDeviceConfigImpl(const DeviceTypePtr& deviceType, Partial);

    ErrCode INTERFACE_FUNC getDeviceType(IDeviceType** type) override;
    ErrCode INTERFACE_FUNC getTypeConfig(IPropertyObject** config) override;
    ErrCode INTERFACE_FUNC getPrimaryAddressType(AddressType* type) override;
    ErrCode INTERFACE_FUNC setPrimaryAddressType(AddressType type) override;
    ErrCode INTERFACE_FUNC getScanOnAdd(Bool* scanOnAdd) override;
    ErrCode INTERFACE_FUNC setScanOnAdd(Bool scanOnAdd) override;
    ErrCode INTERFACE_FUNC getStreamingOptions(IAddStreamingOptions** options) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

private:
    static bool takesStreamings(const DeviceTypePtr& deviceType);
    DeviceTypePtr deviceType;
};

END_NAMESPACE_OPENDAQ
