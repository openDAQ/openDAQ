#include "opendaq/mock/mock_device_module.h"
#include "opendaq/mock/mock_physical_device.h"
#include "opendaq/mock/mock_fb_module.h"
#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/instance_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/module_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>

using namespace daq;

MockDeviceModuleImpl::MockDeviceModuleImpl(daq::ContextPtr ctx)
    : ctx(std::move(ctx))
{
}

ErrCode MockDeviceModuleImpl::getAvailableDevices(IList** availableDevices)
{
    ListPtr<IDeviceInfo> availableDevicesPtr = List<IDeviceInfo>();

    auto daqClientDeviceInfo = DeviceInfo("daqmock://client_device");
    daqClientDeviceInfo.setDeviceType(DeviceType("mock_client_device", "Client", "Client device", "daqmock"));
    availableDevicesPtr.pushBack(daqClientDeviceInfo);

    auto mockPhysDeviceInfo = DeviceInfo("daqmock://phys_device");
    mockPhysDeviceInfo.setDeviceType(DeviceType("mock_phys_device", "Mock physical device", "Mock", "daqmock"));
    mockPhysDeviceInfo.setManufacturer("openDAQ");
    mockPhysDeviceInfo.setSerialNumber("mock_phys_ser");
    availableDevicesPtr.pushBack(mockPhysDeviceInfo);

    *availableDevices = availableDevicesPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    auto mockConfig = PropertyObject();
    mockConfig.addProperty(StringProperty("message", ""));

    mockConfig.addProperty(BoolProperty("netConfigEnabled", False));
    mockConfig.addProperty(ListProperty("ifaceNames", List<IString>()));

    auto submitArguments = List<IArgumentInfo>(ArgumentInfo("ifaceName", ctString), ArgumentInfo("config", ctObject));
    mockConfig.addProperty(FunctionProperty("onSubmitConfig", ProcedureInfo(submitArguments)));

    auto retrieveArguments = List<IArgumentInfo>(ArgumentInfo("ifaceName", ctString));
    mockConfig.addProperty(FunctionProperty("onRetrieveConfig", FunctionInfo(ctObject, retrieveArguments)));

    auto types = Dict<IString, IDeviceType>();
    types.set("mock_client_device", DeviceType("mock_client_device", "Client", "Client device", "daqmock", mockConfig));
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
        devicePtr = DevicePtr(MockPhysicalDevice_Create(ctx, parent, StringPtr(id), config));
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Invalid connection string %s", connStr.getCharPtr());
    }
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

ErrCode MockDeviceModuleImpl::createServer(IServer** server,
                                           IString* /*serverType*/,
                                           IDevice* /*rootDevice*/,
                                           IPropertyObject* /*config*/)
{
    *server = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockDeviceModuleImpl::getModuleInfo(daq::IModuleInfo** info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    *info = ModuleInfo(VersionInfo(0, 0, 0), "MockModule", "mock").detach();
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
