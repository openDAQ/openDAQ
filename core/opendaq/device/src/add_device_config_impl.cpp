#include <opendaq/add_device_config_impl.h>
#include <opendaq/address_info_utils.h>
#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/add_streaming_options_impl.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/container_utils.h>
#include <opendaq/streaming_type_utils.h>
#include <coreobjects/property_factory.h>
#include <opendaq/component_type_private_ptr.h>
#include <coreobjects/eval_value_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/authentication_config_ptr.h>

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
static const char* SelectedTransportOptionKey = "selectedTransportOption";

// The options of `options` that the type lists as well.
// They keep the order and ports of `options`. The security mode and certificate requirement come from the type's option.
DictPtr<IString, ITransportOption> knownTransportOptions(const DictPtr<IString, ITransportOption>& options,
                                                         const DictPtr<IString, ITransportOption>& typeOptions)
{
    auto known = Dict<IString, ITransportOption>();
    for (const auto& [id, option] : options)
    {
        if (!typeOptions.hasKey(id))
            continue;

        const TransportOptionPtr typeOption = typeOptions.get(id);
        known.set(id, daq::TransportOption(id, option.getPort(), typeOption.getSecurityMode(), typeOption.getCertificateRequired()));
    }

    return known;
}

// The transport options that `capabilities` announce for a type and the type lists as well.
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

        return knownTransportOptions(announced, typeOptions);
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

// Copies the supplied credential - which are never serialized - between the authentication configs of two add-device/add-streaming configs
void copySuppliedCredential(const PropertyObjectPtr& from, const PropertyObjectPtr& to)
{
    if (!from.assigned() || !to.assigned() || !from.hasProperty(AuthenticationConfig) || !to.hasProperty(AuthenticationConfig))
        return;

    const AuthenticationConfigPtr source = from.getPropertyValue(AuthenticationConfig);
    const AuthenticationConfigPtr target = to.getPropertyValue(AuthenticationConfig);
    target.setSuppliedCredential(source.getSuppliedCredential());
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
    const auto streamingOptions = [&capabilities](const StreamingTypePtr& streamingType)
    {
        return add_device_config::announcedTransportOptions(
            capabilities, streamingType.getConnectionStringPrefix(), streamingType.getAvailableTransportOptions());
    };
    addConfigProperties(streamingTypes, streamingOptions, scanOnAdd);
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType,
                                         const DictPtr<IString, ITransportOption>& transportOptions,
                                         Partial)
    : Super()
    , deviceType(deviceType)
    , transportOptions(FrozenClone(transportOptions.assigned() ? transportOptions : Dict<IString, ITransportOption>()))
{
}

void AddDeviceConfigImpl::addConfigProperties(const DictPtr<IString, IStreamingType>& streamingTypes,
                                              const StreamingTransportOptions& streamingOptions,
                                              Bool scanOnAdd)
{
    Super::addProperty(StringPropertyBuilder(add_device_config::DeviceType, deviceType.getId()).setReadOnly(true).build());
    Super::addProperty(ObjectProperty(add_device_config::TypeConfig, deviceType.createDefaultConfig()));
    Super::addProperty(SparseSelectionProperty(
        add_device_config::PrimaryAddressType, add_device_config::addressTypeNames(), static_cast<Int>(AddressType::IPv4)));
    Super::addProperty(BoolProperty(add_device_config::ScanOnAdd, scanOnAdd));
    if (this->deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_device_config::AuthenticationConfig, deviceType.createAuthenticationConfig()));

    // The selected transport option, and the port to connect at, which is the one of the selected option until it is set
    if (this->transportOptions.getCount() > 0)
    {
        const TransportOptionPtr firstOption = this->transportOptions[0];
        Super::addProperty(StringPropertyBuilder(add_device_config::TransportOption, firstOption.getName())
                               .setSelectionValues(TransportOptionNames(this->transportOptions))
                               .build());
        Super::addProperty(IntProperty(
            add_device_config::Port, EvalValue(SelectedTransportOptionPortEval(this->transportOptions, add_device_config::TransportOption))));
    }

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
        const auto typeOptions = streamingOptions(streamingType);
        streamingConfigs.set(typeId, createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType, typeOptions));
    }

    const auto options = createWithImplementation<IAddStreamingOptions, AddStreamingOptionsImpl>(streamingConfigs);
    Super::addProperty(ObjectProperty(add_device_config::Streaming, options));
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
            config = BindSaved(saved, deviceTypes, streamingTypes, scanOnAddDefault, objPtr.asPtr<IAddDeviceConfig>());
        };
        JsonDeserializer().callCustomProc(bindSaved, serializer.getOutput());

        // The round trip drops the supplied credentials, so they are carried over directly
        add_device_config::copySuppliedCredential(objPtr, config);
        const auto options = objPtr.asPtr<IAddDeviceConfig>().getStreamingOptions();
        const auto boundOptions = config.getStreamingOptions();
        if (options.assigned() && boundOptions.assigned())
            add_device_config::copySuppliedCredential(options.getStreamingConfig(), boundOptions.getStreamingConfig());

        *bound = config.detach();
    });
}

ErrCode AddDeviceConfigImpl::trim(IAddDeviceConfig** trimmed)
{
    OPENDAQ_PARAM_NOT_NULL(trimmed);

    return daqTry([&]
    {
        PropertyObjectPtr cloned;
        checkErrorInfo(clone(&cloned));

        AddDeviceConfigPtr copy = cloned;
        const auto options = copy.getStreamingOptions();
        if (options.assigned())
        {
            const auto optionsImpl = dynamic_cast<AddStreamingOptionsImpl*>(options.getObject());
            if (optionsImpl)
                optionsImpl->removeUnselected();
        }

        *trimmed = copy.detach();
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
    return BindSaved(saved, deviceTypes, streamingTypes, scanOnAdd, nullptr);
}

AddDeviceConfigPtr AddDeviceConfigImpl::BindSaved(const SerializedObjectPtr& saved,
                                                  const DictPtr<IString, IDeviceType>& deviceTypes,
                                                  const DictPtr<IString, IStreamingType>& streamingTypes,
                                                  Bool scanOnAdd,
                                                  const AddDeviceConfigPtr& source)
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

        auto sourceStreamingConfigs = Dict<IString, IAddStreamingConfig>();
        if (source.assigned())
        {
            if (const auto sourceOptions = source.getStreamingOptions(); sourceOptions.assigned())
                sourceStreamingConfigs = sourceOptions.getAvailableStreamingConfigs();
        }

        const auto streamingOptions = [&sourceStreamingConfigs](const StreamingTypePtr& streamingType)
        {
            const auto typeOptions = streamingType.getAvailableTransportOptions();
            if (!sourceStreamingConfigs.hasKey(streamingType.getId()))
                return typeOptions;

            return add_device_config::knownTransportOptions(
                sourceStreamingConfigs.get(streamingType.getId()).getAvailableTransportOptions(), typeOptions);
        };

        const auto typeOptions = deviceType.getAvailableTransportOptions();
        const auto deviceOptions =
            source.assigned() ? add_device_config::knownTransportOptions(source.getAvailableTransportOptions(), typeOptions) : typeOptions;

        auto config = createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(deviceType, deviceOptions, Partial());
        dynamic_cast<AddDeviceConfigImpl*>(config.getObject())->addConfigProperties(selectedStreamingType, streamingOptions, scanOnAdd);
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

        // A property value equal to the default is not saved, and the first option is the default.
        // Without this key the first option of the server would become the first option of the type on load.
        if (transportOptions.getCount() > 0)
        {
            const StringPtr selected = objPtr.getPropertyValue(add_device_config::TransportOption);
            serializerPtr.key(add_device_config::SelectedTransportOptionKey);
            serializerPtr.writeString(selected);
        }
    });
}

ErrCode AddDeviceConfigImpl::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    const ErrCode errCode = Super::updateInternal(obj, context);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (errCode == OPENDAQ_IGNORED)
        return errCode;

    return daqTry([&]
    {
        const auto saved = SerializedObjectPtr::Borrow(obj);
        if (saved.hasKey(add_device_config::SelectedTransportOptionKey) && deviceType.getAvailableTransportOptions().getCount() > 0)
            checkErrorInfo(setTransportOption(saved.readString(add_device_config::SelectedTransportOptionKey)));
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

ErrCode AddDeviceConfigImpl::getTransportOptions(IList** transportOptions)
{
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    *transportOptions = this->transportOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddDeviceConfigImpl::setTransportOption(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Device type has no transport options");

    return daqTry([&]
    {
        const StringPtr nameStr = name;
        if (!FindTransportOption(transportOptions, nameStr).assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(There is no transport option "{}" to select)", nameStr));

        auto lock = getRecursiveConfigLock2();
        checkErrorInfo(Super::setPropertyValue(String(add_device_config::TransportOption), nameStr));

        // The port of the newly selected option applies
        checkErrorInfo(Super::clearPropertyValue(String(add_device_config::Port)));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddDeviceConfigImpl::getSelectedTransportOption(ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Device type has no transport options");

    return daqTry([&]
    {
        const StringPtr selected = objPtr.getPropertyValue(add_device_config::TransportOption);
        TransportOptionPtr option = FindTransportOption(transportOptions, selected);
        if (!option.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(The selected transport option "{}" is not one of the options)", selected));

        *transportOption = option.detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddDeviceConfigImpl::setPort(IInteger* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Device type has no transport options");

    if (static_cast<Int>(IntegerPtr(port)) < 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The port must not be negative");

    return Super::setPropertyValue(String(add_device_config::Port), port);
}

ErrCode AddDeviceConfigImpl::getPort(IInteger** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Device type has no transport options");

    return daqTry([&] { *port = objPtr.getPropertyValue(add_device_config::Port).asPtr<IInteger>().detach(); });
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
