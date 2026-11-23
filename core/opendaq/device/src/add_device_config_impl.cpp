#include <opendaq/add_device_config_impl.h>
#include <opendaq/address_info_utils.h>
#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/add_streaming_options_impl.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/container_utils.h>
#include <opendaq/streaming_type_utils.h>
#include <coreobjects/property_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/component_type_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_device_config
{
static const char* DeviceType = "DeviceType";
static const char* TypeConfig = "TypeConfig";
static const char* PrimaryAddressType = "PrimaryAddressType";
static const char* ScanOnAdd = "ScanOnAdd";
static const char* Streaming = "Streaming";
static const char* AuthenticationConfig = "AuthenticationConfig";
static const char* TransportOption = "TransportOption";
static const char* Port = "Port";

static const char* DeviceTypeKey = "deviceType";
static const char* DeviceTypeIdKey = "deviceTypeId";
static const char* PropValuesKey = "propValues";
static const char* AvailableTransportOptionsKey = "availableTransportOptions";

// The transport options that `capabilities` announce for a type and the type lists as well.
// They keep the server's order and ports. The security mode and certificate requirement come from the type's option.
// Returns the type's own options when the server announces none, and an empty dict when the type knows none of them.
DictPtr<IString, ITransportOption> announcedTransportOptions(const ListPtr<IServerCapability>& capabilities,
                                                             const StringPtr& prefix,
                                                             const DictPtr<IString, ITransportOption>& typeOptions)
{
    if (!capabilities.assigned())
        return typeOptions;

    for (const auto& capability : capabilities)
    {
        if (getServerCapabilityPrefix(capability) != prefix)
            continue;

        const auto announced = capability.getAvailableTransportOptions();
        if (!announced.assigned() || announced.getCount() == 0)
            continue;

        auto known = Dict<IString, ITransportOption>();
        for (const auto& [id, option] : announced)
        {
            if (!typeOptions.hasKey(id))
                continue;

            const TransportOptionPtr typeOption = typeOptions.get(id);
            known.set(id, daq::TransportOption(id, option.getPort(), typeOption.getSecurityMode(), typeOption.getCertificateRequired()));
        }

        return known;
    }

    return typeOptions;
}

DictPtr<Int, IString> addressTypeNames()
{
    auto names = Dict<Int, IString>();
    for (const auto type : {AddressType::IPv4, AddressType::IPv6})
        names.set(static_cast<Int>(type), getAddressTypeName(type));
    return names;
}
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType,
                                         const DictPtr<IString, IStreamingType>& streamingTypes,
                                         const ListPtr<IServerCapability>& capabilities,
                                         Bool scanOnAdd)
    : AddDeviceConfigImpl(deviceType,
                          add_device_config::announcedTransportOptions(
                              capabilities, deviceType.getConnectionStringPrefix(), deviceType.getAvailableTransportOptions()),
                          Partial())
{
    Super::addProperty(StringPropertyBuilder(add_device_config::DeviceType, deviceType.getId()).setReadOnly(true).build());
    Super::addProperty(ObjectProperty(add_device_config::TypeConfig, deviceType.createDefaultConfig()));
    Super::addProperty(SparseSelectionProperty(
        add_device_config::PrimaryAddressType, add_device_config::addressTypeNames(), static_cast<Int>(AddressType::IPv4)));
    Super::addProperty(BoolProperty(add_device_config::ScanOnAdd, scanOnAdd));

    if (this->transportOptions.getCount() > 0)
    {
        const TransportOptionPtr firstOption = FirstValue(this->transportOptions);
        Super::addProperty(StringPropertyBuilder(add_device_config::TransportOption, firstOption.getId())
                               .setSelectionValues(this->transportOptions.getKeyList())
                               .build());
    }

    Super::addProperty(IntPropertyBuilder(add_device_config::Port, -1).setMinValue(-1).build());

    if (!takesStreamings(deviceType) || !streamingTypes.assigned() || streamingTypes.getCount() == 0)
        return;

    auto streamingConfigs = Dict<IString, IAddStreamingConfig>();
    for (const auto& [typeId, streamingType] : streamingTypes)
    {
        const auto options = add_device_config::announcedTransportOptions(
            capabilities, streamingType.getConnectionStringPrefix(), streamingType.getAvailableTransportOptions());
        streamingConfigs.set(typeId, createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType, options));
    }

    const auto options = createWithImplementation<IAddStreamingOptions, AddStreamingOptionsImpl>(streamingConfigs);
    Super::addProperty(ObjectProperty(add_device_config::Streaming, options));
    if (this->deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_device_config::AuthenticationConfig, deviceType.createAuthenticationConfig()));
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType,
                                         const DictPtr<IString, ITransportOption>& transportOptions,
                                         Partial)
    : Super()
    , deviceType(deviceType)
    , transportOptions(FrozenClone(transportOptions.assigned() ? transportOptions : Dict<IString, ITransportOption>()))
{
}

ErrCode AddDeviceConfigImpl::getDeviceType(IDeviceType** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = deviceType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddDeviceConfigImpl::getTypeConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&] { *config = objPtr.getPropertyValue(add_device_config::TypeConfig).asPtr<IPropertyObject>().detach(); });
}

ErrCode AddDeviceConfigImpl::getPrimaryAddressType(AddressType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    return daqTry([&]
    {
        const Int value = objPtr.getPropertyValue(add_device_config::PrimaryAddressType);
        *type = static_cast<AddressType>(value);
    });
}

ErrCode AddDeviceConfigImpl::setPrimaryAddressType(AddressType type)
{
    return Super::setPropertyValue(String(add_device_config::PrimaryAddressType), Integer(static_cast<Int>(type)));
}

ErrCode AddDeviceConfigImpl::getScanOnAdd(Bool* scanOnAdd)
{
    OPENDAQ_PARAM_NOT_NULL(scanOnAdd);

    return daqTry([&] { *scanOnAdd = static_cast<Bool>(objPtr.getPropertyValue(add_device_config::ScanOnAdd)); });
}

ErrCode AddDeviceConfigImpl::setScanOnAdd(Bool scanOnAdd)
{
    return Super::setPropertyValue(String(add_device_config::ScanOnAdd), Boolean(scanOnAdd));
}

ErrCode AddDeviceConfigImpl::getStreamingOptions(IAddStreamingOptions** options)
{
    OPENDAQ_PARAM_NOT_NULL(options);

    return daqTry([&]
    {
        if (!objPtr.hasProperty(add_device_config::Streaming))
        {
            *options = nullptr;
            return;
        }

        *options = objPtr.getPropertyValue(add_device_config::Streaming).asPtr<IAddStreamingOptions>().detach();
    });
}

ErrCode AddDeviceConfigImpl::getAuthenticationConfig(IAuthenticationConfig** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    if (this->deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "This device type supports no authentication method");

    return daqTry([&] { *config = objPtr.getPropertyValue(add_device_config::AuthenticationConfig).asPtr<IAuthenticationConfig>().detach(); });
}

ErrCode AddDeviceConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddDeviceConfigImpl>(deviceType, transportOptions, Partial());
        dynamic_cast<AddDeviceConfigImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
        *cloned = obj.detach();
    });
}

ErrCode AddDeviceConfigImpl::bind(IDict* deviceTypes, IDict* streamingTypes, IAddDeviceConfig** bound)
{
    OPENDAQ_PARAM_NOT_NULL(bound);

    return daqTry([&]
    {
        // A JSON round trip through the save form reuses the bind of a saved config instead of a separate copy path.
        const auto serializer = JsonSerializer();
        checkErrorInfo(serializeForUpdate(serializer));

        const Bool scanOnAddDefault = objPtr.getProperty(add_device_config::ScanOnAdd).getDefaultValue();
        AddDeviceConfigPtr config;
        const auto bindSaved = [&](const SerializedObjectPtr& saved)
        {
            config = Bind(saved, deviceTypes, streamingTypes, scanOnAddDefault);
        };
        JsonDeserializer().callCustomProc(bindSaved, serializer.getOutput());
        *bound = config.detach();
    });
}

ErrCode AddDeviceConfigImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    Int version;
    OPENDAQ_RETURN_IF_FAILED(serializer->getVersion(&version));
    if (version < 4)
        return daqTry([&] { toLegacyAddDeviceConfig(objPtr.asPtr<IAddDeviceConfig>()).serialize(serializer); });
    return Super::serialize(serializer);
}

ErrCode AddDeviceConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AddDeviceConfigImpl::SerializeId()
{
    return "AddDeviceConfig";
}

ErrCode AddDeviceConfigImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto createConfig =
            [&factoryCallback](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& /*className*/)
        {
            const DeviceTypePtr deviceType = serialized.readObject(add_device_config::DeviceTypeKey, context, factoryCallback);

            const DictPtr<IString, ITransportOption> options =
                serialized.hasKey(add_device_config::AvailableTransportOptionsKey)
                    ? DictPtr<IString, ITransportOption>(
                          serialized.readObject(add_device_config::AvailableTransportOptionsKey, context, factoryCallback).asPtr<IDict>())
                    : Dict<IString, ITransportOption>();

            return createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(deviceType, options, Partial());
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createConfig).detach();
    });
}

AddDeviceConfigPtr AddDeviceConfigImpl::Bind(const SerializedObjectPtr& saved,
                                             const DictPtr<IString, IDeviceType>& deviceTypes,
                                             const DictPtr<IString, IStreamingType>& streamingTypes,
                                             Bool scanOnAdd)
{
    if (!saved.hasKey("__type") || saved.readString("__type") != SerializeId())
        return nullptr;

    const auto givenStreamingTypes = streamingTypes.assigned() ? streamingTypes : Dict<IString, IStreamingType>();
    const StringPtr deviceTypeId = saved.readString(add_device_config::DeviceTypeIdKey);
    if (!deviceTypes.assigned() || !deviceTypes.hasKey(deviceTypeId))
        DAQ_THROW_EXCEPTION(NotFoundException, "Device type \"{}\" is not available", deviceTypeId);

    try
    {
        const auto deviceType = deviceTypes.get(deviceTypeId);
        const auto savedValues = saved.readSerializedObject(add_device_config::PropValuesKey);
        auto selectedStreamingType = Dict<IString, IStreamingType>();
        if (savedValues.hasKey(add_device_config::Streaming))
        {
            const auto savedOptions = savedValues.readSerializedObject(add_device_config::Streaming);
            const auto typeId = AddStreamingOptionsImpl::SavedStreamingType(savedOptions);
            if (givenStreamingTypes.hasKey(typeId))
                selectedStreamingType.set(typeId, givenStreamingTypes.get(typeId));
        }

        auto config =
            createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(deviceType, selectedStreamingType, nullptr, scanOnAdd);
        checkErrorInfo(config.asPtr<IUpdatable>()->updateInternal(saved, nullptr));
        return config;
    }
    catch (const DaqException& e)
    {
        DAQ_THROW_EXCEPTION(InvalidValueException, "{}", e.what());
    }
}

ErrCode AddDeviceConfigImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));

    return daqTry([&]
    {
        const auto serializerPtr = SerializerPtr::Borrow(serializer);
        if (!forUpdate)
        {
            serializerPtr.key(add_device_config::DeviceTypeKey);
            deviceType.asPtr<ISerializable>().serialize(serializerPtr);

            // The options may be the ones the server of the device announces, which the Device type does not list
            if (transportOptions.getCount() > 0)
            {
                serializerPtr.key(add_device_config::AvailableTransportOptionsKey);
                transportOptions.serialize(serializerPtr);
            }

            return;
        }

        serializerPtr.key(add_device_config::DeviceTypeIdKey);
        serializerPtr.writeString(deviceType.getId());
    });
}

ErrCode AddDeviceConfigImpl::getAvailableTransportOptions(IDict** transportOptions)
{
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    *transportOptions = this->transportOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddDeviceConfigImpl::setTransportOption(IString* id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry([&]
    {
        const StringPtr idStr = id;
        if (!transportOptions.hasKey(idStr))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(There is no transport option "{}" to select)", idStr));

        // A port set stays: it overrides the port of whichever option is selected
        return Super::setPropertyValue(String(add_device_config::TransportOption), idStr);
    });
}

ErrCode AddDeviceConfigImpl::getSelectedTransportOption(ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    if (transportOptions.getCount() == 0)
    {
        *transportOption = nullptr;
        return OPENDAQ_SUCCESS;
    }

    // The property only takes the ids of the options
    return daqTry([&]
    {
        const StringPtr selected = objPtr.getPropertyValue(add_device_config::TransportOption);
        *transportOption = transportOptions.get(selected).detach();
    });
}

ErrCode AddDeviceConfigImpl::setPort(Int port)
{
    // -1 drops the port set, so the port of the selected option applies again
    if (port == -1)
        return Super::clearPropertyValue(String(add_device_config::Port));

    if (port < 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The port must not be negative, other than -1");

    return Super::setPropertyValue(String(add_device_config::Port), Integer(port));
}

ErrCode AddDeviceConfigImpl::getPort(Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        // A port set is used whichever option is selected; -1 means none is set
        const Int portSet = objPtr.getPropertyValue(add_device_config::Port);
        if (portSet != -1 || transportOptions.getCount() == 0)
        {
            *port = portSet;
            return;
        }

        const StringPtr selected = objPtr.getPropertyValue(add_device_config::TransportOption);
        *port = transportOptions.get(selected).getPort();
    });
}

bool AddDeviceConfigImpl::takesStreamings(const DeviceTypePtr& deviceType)
{
    switch (deviceType.getDeviceKind())
    {
        case DeviceKind::Unspecified:
        case DeviceKind::Mirrored:
            return true;
        case DeviceKind::Adapter:
        case DeviceKind::Root:
            return false;
    }

    return true;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" ErrCode PUBLIC_EXPORT createAddDeviceConfig(IAddDeviceConfig** objTmp,
                                                       IDeviceType* deviceType,
                                                       IDict* streamingTypes,
                                                       IList* capabilities,
                                                       Bool scanOnAdd)
{
    return daq::createObject<IAddDeviceConfig, AddDeviceConfigImpl>(objTmp, deviceType, streamingTypes, capabilities, scanOnAdd);
}

extern "C" ErrCode PUBLIC_EXPORT createBoundAddDeviceConfig(IAddDeviceConfig** objTmp,
                                                            ISerializedObject* saved,
                                                            IDict* deviceTypes,
                                                            IDict* streamingTypes,
                                                            Bool scanOnAdd)
{
    OPENDAQ_PARAM_NOT_NULL(objTmp);
    OPENDAQ_PARAM_NOT_NULL(saved);

    return daqTry([&] { *objTmp = AddDeviceConfigImpl::Bind(saved, deviceTypes, streamingTypes, scanOnAdd).detach(); });
}

#endif

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddDeviceConfigImpl)

END_NAMESPACE_OPENDAQ
