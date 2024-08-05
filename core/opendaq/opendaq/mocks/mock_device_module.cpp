#include "opendaq/mock/mock_device_module.h"
#include "opendaq/mock/mock_physical_device.h"
#include "opendaq/mock/mock_fb_module.h"
#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/instance_factory.h>
#include <opendaq/device_type_factory.h>

using namespace daq;

MockDeviceModuleImpl::MockDeviceModuleImpl(daq::ContextPtr ctx)
    : ctx(std::move(ctx))
{
}

ErrCode MockDeviceModuleImpl::getName(IString** name)
{
    return createString(name, "MockDeviceModule");
}

ErrCode MockDeviceModuleImpl::getId(IString** id)
{
    return createString(id, "MockDevice");
}

ErrCode MockDeviceModuleImpl::getAvailableDevices(IList** availableDevices)
{
    ListPtr<IDeviceInfo> availableDevicesPtr = List<IDeviceInfo>();

    auto daqClientDeviceInfo = DeviceInfo("daqmock://client_device");
    daqClientDeviceInfo.setDeviceType(DeviceType("mock_client_device", "Client", "Client device", "daqmock"));
    availableDevicesPtr.pushBack(daqClientDeviceInfo);

    auto mockPhysDeviceInfo = DeviceInfo("daqmock://phys_device");
    mockPhysDeviceInfo.setDeviceType(DeviceType("mock_phys_device", "Mock physical device", "Mock", "daqmock"));
    availableDevicesPtr.pushBack(mockPhysDeviceInfo);

    *availableDevices = availableDevicesPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    auto mockConfig = PropertyObject();
    mockConfig.addProperty(StringProperty("message", ""));

    auto types = Dict<IString, IDeviceType>();
    types.set("mock_client_device", DeviceType("mock_client_device", "Client", "Client device", "daqmock"));
    types.set("mock_phys_device", DeviceType("mock_phys_device", "Mock physical device", "Mock", "daqmock", mockConfig));

    *deviceTypes = types.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::createDevice(IDevice** device,
                                           IString* connectionString,
                                           IComponent* parent,
                                           IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(device);
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    StringPtr connStr = connectionString;
    DevicePtr devicePtr;
    if (connStr == "daqmock://client_device")
    {
        const ModulePtr deviceModule(MockDeviceModule_Create(ctx));
        const ModulePtr fbModule(MockFunctionBlockModule_Create(ctx));
        auto manager = ModuleManager("[[none]]");

        manager.addModule(deviceModule);
        manager.addModule(fbModule);

        devicePtr = Client(ctx, "client", nullptr, parent);
    }
    else if (connStr == "daqmock://phys_device")
    {
        std::string id = "mockdev";
        if (cnt != 0)
            id += std::to_string(cnt);
        cnt++;
        devicePtr = MockPhysicalDevice_Create(ctx, parent, StringPtr(id), config);
    }
    else
    {
        return OPENDAQ_ERR_INVALIDPARAMETER;
    }

    ServerCapabilityConfigPtr connectionInfo = devicePtr.getInfo().getConfigurationConnectionInfo();
    connectionInfo.setPrefix("daqmock://")
                  .setConnectionString(connectionString)
                  .freeze();
    
    *device = devicePtr.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::getAvailableFunctionBlockTypes(IDict** types)
{
    auto list = Dict<IString, IFunctionBlockType>();
    *types = list.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* /*id*/, IComponent* /*parent*/, IString* /*localId*/, IPropertyObject* /*config*/)
{
    *functionBlock = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::getAvailableServerTypes(IDict** serverTypes)
{
    *serverTypes = Dict<IString, IServerType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::createServer(IServer** server, IString* /*serverType*/, IDevice* /*rootDevice*/, IPropertyObject* /*config*/)
{
    *server = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::getVersionInfo(IVersionInfo** version)
{
    if (version == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *version = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                              IString* /*connectionString*/,
                                              IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockDeviceModuleImpl::completeServerCapability(daq::Bool* /*succeeded*/, daq::IServerCapability* /*source*/, daq::IServerCapabilityConfig* /*target*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockDeviceModuleImpl::getAvailableStreamingTypes(daq::IDict** /*streamingTypes*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockDeviceModule, IModule, IContext*, ctx)
