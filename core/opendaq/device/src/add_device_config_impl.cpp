#include <opendaq/add_device_config_impl.h>
#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/add_streaming_options_impl.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_device_config
{
static const char* DeviceType = "DeviceType";
static const char* TypeConfig = "TypeConfig";
static const char* PrimaryAddressType = "PrimaryAddressType";
static const char* ScanOnAdd = "ScanOnAdd";
static const char* Streaming = "Streaming";

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

#endif

END_NAMESPACE_OPENDAQ
