#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/mock/mock_fb_factory.h>
#include "opendaq/mock/mock_fb_module.h"
#include "opendaq/mock/mock_fb.h"


using namespace daq;

MockFunctionBlockModuleImpl::MockFunctionBlockModuleImpl(daq::ContextPtr ctx)
    : ctx(std::move(ctx))
{
}
ErrCode MockFunctionBlockModuleImpl::getName(IString** name)
{
    return createString(name, "MockFunctionBlockModule");
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

ErrCode MockFunctionBlockModuleImpl::acceptsConnectionParameters(Bool* accepted, IString* /*connectionString*/, IPropertyObject* /*config*/)
{
    *accepted = false;
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

    *functionBlockTypes = typesDict.detach();
    return OPENDAQ_SUCCESS;
}

daq::FunctionBlockTypePtr MockFunctionBlockModuleImpl::CreateDeviceFunctionType()
{
    return FunctionBlockType("mock_fb_uid", "mock_fb", "");
}

ErrCode MockFunctionBlockModuleImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IString* localId, IPropertyObject* /*config*/)
{
    const StringPtr idPtr = id;
    if (idPtr == CreateDeviceFunctionType().getId())
    {
        *functionBlock = MockFunctionBlock(CreateDeviceFunctionType(), ctx, parent, localId).detach();
    }
    else
    {
        return OPENDAQ_ERR_INVALIDPARAMETER;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::getAvailableServerTypes(IDict** serverTypes)
{
    *serverTypes = Dict<IString, IServerType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::createServer(IServer** server, IString* /*serverType*/, IDevice* /*rootDevice*/, IPropertyObject* /*config*/)
{
    *server = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::getVersionInfo(IVersionInfo** version)
{
    if (version == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *version = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::acceptsStreamingConnectionParameters(Bool* accepted,
                                                                          IString* /*connectionString*/,
                                                                          daq::IStreamingInfo* /*config*/)
{
    OPENDAQ_PARAM_NOT_NULL(accepted);

    *accepted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MockFunctionBlockModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                                     IString* /*connectionString*/,
                                                     daq::IStreamingInfo* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockFunctionBlockModule, IModule, IContext*, ctx)
