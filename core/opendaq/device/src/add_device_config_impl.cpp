#include <opendaq/add_device_config_impl.h>
#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/add_streaming_options_impl.h>
#include <coreobjects/property_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_device_config
{
static const char* DeviceType = "DeviceType";
static const char* TypeConfig = "TypeConfig";
static const char* PrimaryAddressType = "PrimaryAddressType";
static const char* ScanOnAdd = "ScanOnAdd";
static const char* Streaming = "Streaming";

static const char* DeviceTypeKey = "deviceType";
static const char* DeviceTypeIdKey = "deviceTypeId";
static const char* ScanOnAddKey = "scanOnAdd";
static const char* PropValuesKey = "propValues";

StringPtr toString(AddressType type)
{
    switch (type)
    {
        case AddressType::IPv4:
            return "IPv4";
        case AddressType::IPv6:
            return "IPv6";
    }

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown address type {}", static_cast<Int>(type));
}

ListPtr<IString> addressTypeNames()
{
    return List<IString>(toString(AddressType::IPv4), toString(AddressType::IPv6));
}
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType,
                                         const DictPtr<IString, IStreamingType>& streamingTypes,
                                         Bool scanOnAdd)
    : AddDeviceConfigImpl(deviceType, Partial())
{
    Super::addProperty(StringPropertyBuilder(add_device_config::DeviceType, deviceType.getId()).setReadOnly(true).build());
    Super::addProperty(ObjectProperty(add_device_config::TypeConfig, deviceType.createDefaultConfig()));
    Super::addProperty(SelectionProperty(
        add_device_config::PrimaryAddressType, add_device_config::addressTypeNames(), static_cast<Int>(AddressType::IPv4)));
    Super::addProperty(BoolProperty(add_device_config::ScanOnAdd, scanOnAdd));

    if (!takesStreamings(deviceType))
        return;

    const auto offeredTypes = streamingTypes.assigned() ? streamingTypes : Dict<IString, IStreamingType>();
    auto streamingConfigs = Dict<IString, IAddStreamingConfig>();
    for (const auto& [typeId, streamingType] : offeredTypes)
        streamingConfigs.set(typeId, createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType));

    const auto options = createWithImplementation<IAddStreamingOptions, AddStreamingOptionsImpl>(streamingConfigs);
    Super::addProperty(ObjectProperty(add_device_config::Streaming, options));
}

AddDeviceConfigImpl::AddDeviceConfigImpl(const DeviceTypePtr& deviceType, Partial)
    : Super()
    , deviceType(deviceType)
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

ErrCode AddDeviceConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddDeviceConfigImpl>(deviceType, Partial());
        const auto impl = dynamic_cast<AddDeviceConfigImpl*>(obj.getObject());
        impl->configureClonedMembers(getCloneParameters());
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

        AddDeviceConfigPtr config;
        JsonDeserializer().callCustomProc([&](const SerializedObjectPtr& saved) { config = Bind(saved, deviceTypes, streamingTypes); },
                                          serializer.getOutput());
        *bound = config.detach();
    });
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
                                             const DictPtr<IString, IStreamingType>& streamingTypes)
{
    const StringPtr deviceTypeId = saved.readString(add_device_config::DeviceTypeIdKey);
    if (!deviceTypes.assigned() || !deviceTypes.hasKey(deviceTypeId))
        DAQ_THROW_EXCEPTION(NotFoundException, "Device type \"{}\" is not available", deviceTypeId);

    try
    {
        const auto deviceType = deviceTypes.get(deviceTypeId);
        auto boundStreamingTypes = Dict<IString, IStreamingType>();
        if (takesStreamings(deviceType))
        {
            const auto savedValues = saved.readSerializedObject(add_device_config::PropValuesKey);
            if (!savedValues.hasKey(add_device_config::Streaming))
                DAQ_THROW_EXCEPTION(InvalidValueException, "Streaming options are missing for device type \"{}\"", deviceTypeId);

            const auto savedStreaming = savedValues.readSerializedObject(add_device_config::Streaming);
            const auto savedOptions = savedStreaming.readSerializedObject(add_device_config::PropValuesKey);
            const auto offeredTypes = streamingTypes.assigned() ? streamingTypes : Dict<IString, IStreamingType>();
            for (const auto& [typeId, streamingType] : offeredTypes)
            {
                if (savedOptions.hasKey(typeId))
                    boundStreamingTypes.set(typeId, streamingType);
            }
        }

        auto config = createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(
            deviceType, boundStreamingTypes, saved.readBool(add_device_config::ScanOnAddKey));
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
        serializerPtr.key(add_device_config::ScanOnAddKey);
        serializerPtr.writeBool(objPtr.getPropertyValue(add_device_config::ScanOnAdd));
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
                                                       Bool scanOnAdd)
{
    return daq::createObject<IAddDeviceConfig, AddDeviceConfigImpl>(objTmp, deviceType, streamingTypes, scanOnAdd);
}

extern "C" ErrCode PUBLIC_EXPORT createBoundAddDeviceConfig(IAddDeviceConfig** objTmp,
                                                            ISerializedObject* saved,
                                                            IDict* deviceTypes,
                                                            IDict* streamingTypes)
{
    OPENDAQ_PARAM_NOT_NULL(objTmp);
    OPENDAQ_PARAM_NOT_NULL(saved);

    return daqTry([&] { *objTmp = AddDeviceConfigImpl::Bind(saved, deviceTypes, streamingTypes).detach(); });
}

#endif

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddDeviceConfigImpl)

END_NAMESPACE_OPENDAQ
