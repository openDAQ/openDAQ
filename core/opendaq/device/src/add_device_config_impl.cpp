#include <opendaq/add_device_config_impl.h>
#include <opendaq/address_info_utils.h>
#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/add_streaming_options_impl.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/container_utils.h>
#include <opendaq/add_config_transport_selection.h>
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

static const char* DeviceTypeKey = "deviceType";
static const char* DeviceTypeIdKey = "deviceTypeId";
static const char* PropValuesKey = "propValues";
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
        known.set(id, TransportOptionBuilderCopy(typeOption).setPort(option.getPort()).build());
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
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType,
                                         const DictPtr<IString, IStreamingType>& streamingTypes,
                                         const ListPtr<IServerCapability>& capabilities,
                                         Bool scanOnAdd)
    : AddDeviceConfigImpl(deviceType, Partial())
{
    const auto streamingOptions = [&capabilities](const StreamingTypePtr& streamingType)
    {
        return add_device_config::announcedTransportOptions(
            capabilities, streamingType.getConnectionStringPrefix(), streamingType.getAvailableTransportOptions());
    };
    addConfigProperties(add_device_config::announcedTransportOptions(
                            capabilities, deviceType.getConnectionStringPrefix(), deviceType.getAvailableTransportOptions()),
                        streamingTypes,
                        streamingOptions,
                        scanOnAdd);
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType, Partial)
    : Super()
    , deviceType(deviceType)
{
}

void AddDeviceConfigImpl::addConfigProperties(const DictPtr<IString, ITransportOption>& transportOptions,
                                              const DictPtr<IString, IStreamingType>& streamingTypes,
                                              const StreamingTransportOptions& streamingOptions,
                                              Bool scanOnAdd)
{
    Super::addProperty(StringPropertyBuilder(add_device_config::DeviceType, deviceType.getId()).setReadOnly(true).build());
    Super::addProperty(ObjectProperty(add_device_config::TypeConfig, deviceType.createDefaultConfig()));
    Super::addProperty(SparseSelectionProperty(
        add_device_config::PrimaryAddressType, add_device_config::addressTypeNames(), static_cast<Int>(AddressType::IPv4)));
    Super::addProperty(BoolProperty(add_device_config::ScanOnAdd, scanOnAdd));

    add_config::AddTransportSelectionProperties(objPtr, transportOptions);

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
    if (this->deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_device_config::AuthenticationConfig, deviceType.createAuthenticationConfig()));
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
        auto obj = createWithImplementation<IPropertyObject, AddDeviceConfigImpl>(deviceType, Partial());
        dynamic_cast<AddDeviceConfigImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
        add_config::FreezeAvailableTransportOptions(obj);
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
            return createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(deviceType, Partial());
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

        auto config = createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(deviceType, Partial());
        dynamic_cast<AddDeviceConfigImpl*>(config.getObject())
            ->addConfigProperties(deviceOptions, selectedStreamingType, streamingOptions, scanOnAdd);
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
            return;
        }

        serializerPtr.key(add_device_config::DeviceTypeIdKey);
        serializerPtr.writeString(deviceType.getId());

        // The base class does not save a value equal to the default, and the default is the first option of this config.
        // A load creates the config from the type, whose first option may be another one.
        // So the selection is always saved, under its own key.
        if (add_config::HasTransportOptions(objPtr))
        {
            const StringPtr selected = objPtr.getPropertyValue(TransportOptionConfigProperty);
            serializerPtr.key(add_device_config::SelectedTransportOptionKey);
            serializerPtr.writeString(selected);
        }
    });
}

ErrCode AddDeviceConfigImpl::serializePropertyValue(const StringPtr& name,
                                                    const ObjectPtr<IBaseObject>& value,
                                                    ISerializer* serializer,
                                                    bool forUpdate)
{
    // The saved form leaves out two properties:
    // - AvailableTransportOptions: a load takes the options from the type, a bind from the source config;
    // - TransportOption: the selection is saved under its own key (see serializeCustomValues).
    if (forUpdate && (name == add_config::AvailableTransportOptionsProperty || name == TransportOptionConfigProperty))
        return OPENDAQ_SUCCESS;

    return Super::serializePropertyValue(name, value, serializer, forUpdate);
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
        // A selection that the loaded config does not list fails the load, also for a type without options.
        // The device must not change its transport silently.
        if (saved.hasKey(add_device_config::SelectedTransportOptionKey))
            checkErrorInfo(setTransportOption(saved.readString(add_device_config::SelectedTransportOptionKey)));
    });
}

ErrCode AddDeviceConfigImpl::getAvailableTransportOptions(IDict** transportOptions)
{
    return add_config::GetAvailableTransportOptions(objPtr, transportOptions);
}

ErrCode AddDeviceConfigImpl::setTransportOption(IString* id)
{
    return add_config::SetTransportOption(objPtr, id);
}

ErrCode AddDeviceConfigImpl::getSelectedTransportOption(ITransportOption** transportOption)
{
    return add_config::GetSelectedTransportOption(objPtr, transportOption);
}

ErrCode AddDeviceConfigImpl::setPort(Int port)
{
    return add_config::SetPort(objPtr, port);
}

ErrCode AddDeviceConfigImpl::getPort(Int* port)
{
    return add_config::GetPort(objPtr, port);
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
