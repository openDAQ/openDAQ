#include <opendaq/network_interface_impl.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

NetworkInterfaceImpl::NetworkInterfaceImpl(const StringPtr& name,
                                           const StringPtr& ownerDeviceManufacturerName,
                                           const StringPtr& ownerDeviceSerialNumber,
                                           const BaseObjectPtr& moduleManager)
    : interfaceName(name)
    , ownerDeviceManufacturerName(ownerDeviceManufacturerName)
    , ownerDeviceSerialNumber(ownerDeviceSerialNumber)
    , moduleManager(moduleManager.asPtrOrNull<IModuleManagerUtils>(true))
{
    if (!this->interfaceName.assigned() || this->interfaceName == "" ||
        !this->ownerDeviceManufacturerName.assigned() || this->ownerDeviceManufacturerName == "" ||
        !this->ownerDeviceSerialNumber.assigned() || this->ownerDeviceSerialNumber == "" ||
        !this->moduleManager.assigned())
        throw InvalidParameterException("Cannot create NetworkInterface object - invalid parameters");
}

ErrCode NetworkInterfaceImpl::requestCurrentConfiguration(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode NetworkInterfaceImpl::submitConfiguration(IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return moduleManager->changeIpConfig(interfaceName, ownerDeviceManufacturerName, ownerDeviceSerialNumber, config);
}

ErrCode NetworkInterfaceImpl::createDefaultConfiguration(IPropertyObject** defaultConfig)
{
    auto config = PropertyObject();

    config.addProperty(BoolProperty("dhcp4", True));
    config.addProperty(ListProperty("addresses4", List<IString>()));
    config.addProperty(StringProperty("gateway4", ""));
    config.addProperty(BoolProperty("dhcp6", True));
    config.addProperty(ListProperty("addresses6", List<IString>()));
    config.addProperty(StringProperty("gateway6", ""));

    *defaultConfig = config.detach();

    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createNetworkInterface(
        INetworkInterface** objTmp,
        IString* name,
        IString* ownerDeviceManufacturerName,
        IString* ownerDeviceSerialNumber,
        IBaseObject* moduleManager)
{
    return createObject<INetworkInterface, NetworkInterfaceImpl, IString*, IString*, IString*, IBaseObject*>(
        objTmp,
        name,
        ownerDeviceManufacturerName,
        ownerDeviceSerialNumber,
        moduleManager);
}

#endif

END_NAMESPACE_OPENDAQ
