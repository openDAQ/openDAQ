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

    auto daqClientDeviceInfo = DeviceInfo("daq_client_device");
    daqClientDeviceInfo.setDeviceType(DeviceType("daq_client_device", "Client", "Client device"));
    availableDevicesPtr.pushBack(daqClientDeviceInfo);

    auto mockPhysDeviceInfo = DeviceInfo("mock_phys_device");
    mockPhysDeviceInfo.setDeviceType(DeviceType("mock_phys_device", "Mock physical device", "Mock"));
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
    types.set("daq_client_device", DeviceType("daq_client_device", "Client", "Client device"));
    types.set("mock_phys_device", DeviceType("mock_phys_device", "Mock physical device", "Mock", mockConfig));

    *deviceTypes = types.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::acceptsConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* /*config*/)
{
    const StringPtr connStr = connectionString;

    *accepted = false;
    if (connStr == "daq_client_device" || connStr == "mock_phys_device")
        *accepted = true;

    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::createDevice(IDevice** device,
                                           IString* connectionString,
                                           IComponent* parent,
                                           IPropertyObject* config)
{
    StringPtr connStr = connectionString;
    if (connStr == "daq_client_device")
    {
        const ModulePtr deviceModule(MockDeviceModule_Create(ctx));
        const ModulePtr fbModule(MockFunctionBlockModule_Create(ctx));
        auto manager = ModuleManager("[[none]]");

        manager.addModule(deviceModule);
        manager.addModule(fbModule);

        auto clientDevice = Client(ctx, "client", nullptr, parent);
        *device = clientDevice.detach();
    }
    else if (connStr == "mock_phys_device")
    {
        std::string id = "mockdev";
        if (cnt != 0)
            id += std::to_string(cnt);
        cnt++;
        DevicePtr physicalDevice(MockPhysicalDevice_Create(ctx, parent, StringPtr(id), config));
        *device = physicalDevice.detach();
    }
    else
    {
        return OPENDAQ_ERR_INVALIDPARAMETER;
    }

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

ErrCode MockDeviceModuleImpl::acceptsStreamingConnectionParameters(Bool* accepted,
                                                                   IString* /*connectionString*/,
                                                                   IPropertyObject* /*config*/)
{
    OPENDAQ_PARAM_NOT_NULL(accepted);

    *accepted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                              IString* /*connectionString*/,
                                              IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockDeviceModuleImpl::createConnectionString(IString** /*connectionString*/, IServerCapability* /*serverCapability*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockDeviceModule, IModule, IContext*, ctx)
