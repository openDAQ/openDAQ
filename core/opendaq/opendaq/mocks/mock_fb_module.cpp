#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/mock/mock_fb_factory.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/mock_fb.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/module_info_factory.h>

using namespace daq;

MockFunctionBlockModuleImpl::MockFunctionBlockModuleImpl(daq::ContextPtr ctx)
    : ctx(std::move(ctx))
{
}

ErrCode INTERFACE_FUNC MockFunctionBlockModuleImpl::getModuleInfo(daq::IModuleInfo** info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    *info = ModuleInfo(VersionInfo(0, 0, 0), "MockModule", "mock").detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::getAvailableDevices(IList** availableDevices)
{
    ListPtr<IDeviceInfo> availableDevicesPtr = List<IDeviceInfo>();
    *availableDevices = availableDevicesPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    *deviceTypes = Dict<IString, IDeviceType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::createDevice(IDevice** device,
                                                  IString* /*connectionString*/,
                                                  IComponent* /*parent*/,
                                                  IPropertyObject* /*config*/)
{
    *device = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    auto typesDict = Dict<IString, IFunctionBlockType>();

    auto type = CreateDeviceFunctionType();
    typesDict.set(type.getId(), type);

    auto mockFbDynamicInputPortsType = MockFunctionBlockDynamicInputPortImpl::CreateType();
    typesDict.set(mockFbDynamicInputPortsType.getId(), mockFbDynamicInputPortsType);

    auto mockFbDynamicOutputPortsType = MockFunctionBlockDynamicOutputPortImpl::CreateType();
    typesDict.set(mockFbDynamicOutputPortsType.getId(), mockFbDynamicOutputPortsType);

    *functionBlockTypes = typesDict.detach();
    return OPENDAQ_SUCCESS;
}

daq::FunctionBlockTypePtr MockFunctionBlockModuleImpl::CreateDeviceFunctionType()
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(IntProperty("TestConfigInt", 0));
    defaultConfig.addProperty(StringProperty("TestConfigString", ""));

    return FunctionBlockType("mock_fb_uid", "mock_fb", "", defaultConfig);
}

ErrCode MockFunctionBlockModuleImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IString* localId, IPropertyObject* config)
{
    const auto type = CreateDeviceFunctionType();
    const StringPtr idPtr = id;

    if (idPtr == type.getId())
    {
        *functionBlock = MockFunctionBlock(CreateDeviceFunctionType(), ctx, parent, localId, config).detach();
    }
    else if (idPtr == MockFunctionBlockDynamicInputPortImpl::CreateType().getId())
    {
        *functionBlock = MockFunctionBlockDynamicInputPort(MockFunctionBlockDynamicInputPortImpl::CreateType(), ctx, parent, localId, config).detach();
    }
    else if (idPtr == MockFunctionBlockDynamicOutputPortImpl::CreateType().getId())
    {
        *functionBlock = MockFunctionBlockDynamicOutputPort(MockFunctionBlockDynamicOutputPortImpl::CreateType(), ctx, parent, localId, config).detach();
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::getAvailableServerTypes(IDict** serverTypes)
{
    *serverTypes = Dict<IString, IServerType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::createServer(IServer** server,
                                                  IString* /*serverType*/,
                                                  IDevice* /*rootDevice*/,
                                                  IPropertyObject* /*config*/)
{
    *server = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                                     IString* /*connectionString*/,
                                                     IPropertyObject* /*config*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode MockFunctionBlockModuleImpl::completeServerCapability(daq::Bool* /*succeeded*/, daq::IServerCapability* /*source*/, daq::IServerCapabilityConfig* /*target*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

daq::ErrCode MockFunctionBlockModuleImpl::getAvailableStreamingTypes(daq::IDict** /*streamingTypes*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockFunctionBlockModule, IModule, IContext*, ctx)
