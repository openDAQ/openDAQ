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
#include <opendaq/add_device_config_private.h>
#include <opendaq/transport_option_ptr.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class AddDeviceConfigImpl : public GenericPropertyObjectImpl<IPropertyObject, IAddDeviceConfig, IAddDeviceConfigPrivate>
{
    class Partial
    {
        Partial()
        {
        }
        friend class AddDeviceConfigImpl;
    };

public:
    using Super = GenericPropertyObjectImpl<IPropertyObject, IAddDeviceConfig, IAddDeviceConfigPrivate>;

    AddDeviceConfigImpl(const DeviceTypePtr& deviceType, const DictPtr<IString, IStreamingType>& streamingTypes, Bool scanOnAdd);

    // Lists the transport options the server of a device announces instead of the ones of the types: `transportOptions`
    // for the Device type and `streamingTransportOptions` by Streaming type ID. A type without them keeps its own.
    AddDeviceConfigImpl(const DeviceTypePtr& deviceType,
                        const DictPtr<IString, IStreamingType>& streamingTypes,
                        Bool scanOnAdd,
                        const ListPtr<ITransportOption>& transportOptions,
                        const DictPtr<IString, IList>& streamingTransportOptions);
    AddDeviceConfigImpl(const DeviceTypePtr& deviceType, const ListPtr<ITransportOption>& transportOptions, Partial);

    ErrCode INTERFACE_FUNC getDeviceType(IDeviceType** type) override;
    ErrCode INTERFACE_FUNC getTypeConfig(IPropertyObject** config) override;
    ErrCode INTERFACE_FUNC getPrimaryAddressType(AddressType* type) override;
    ErrCode INTERFACE_FUNC setPrimaryAddressType(AddressType type) override;
    ErrCode INTERFACE_FUNC getScanOnAdd(Bool* scanOnAdd) override;
    ErrCode INTERFACE_FUNC setScanOnAdd(Bool scanOnAdd) override;
    ErrCode INTERFACE_FUNC getStreamingOptions(IAddStreamingOptions** options) override;
    ErrCode INTERFACE_FUNC getAuthenticationConfig(IAuthenticationConfig** config) override;
    ErrCode INTERFACE_FUNC getTransportOptions(IList** transportOptions) override;
    ErrCode INTERFACE_FUNC setTransportOption(IString* name) override;
    ErrCode INTERFACE_FUNC getSelectedTransportOption(ITransportOption** transportOption) override;
    ErrCode INTERFACE_FUNC setPort(IInteger* port) override;
    ErrCode INTERFACE_FUNC getPort(IInteger** port) override;

    ErrCode INTERFACE_FUNC bind(IDict* deviceTypes, IDict* streamingTypes, IAddDeviceConfig** bound) override;
    ErrCode INTERFACE_FUNC trim(IAddDeviceConfig** trimmed) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    static AddDeviceConfigPtr Bind(const SerializedObjectPtr& saved,
                                   const DictPtr<IString, IDeviceType>& deviceTypes,
                                   const DictPtr<IString, IStreamingType>& streamingTypes,
                                   Bool scanOnAdd);

protected:
    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;

private:
    static bool takesStreamings(const DeviceTypePtr& deviceType);

    DeviceTypePtr deviceType;
    // The options the server of the device announces, else the Device type's
    ListPtr<ITransportOption> transportOptions;
};

END_NAMESPACE_OPENDAQ
