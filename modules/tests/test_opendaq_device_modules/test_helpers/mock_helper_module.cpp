#include "mock_helper_module.h"
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{

MockHelperModuleImpl::MockHelperModuleImpl(ContextPtr ctx, FunctionPtr availableDevicesCb)
    : ctx(std::move(ctx))
    , availableDevicesCb(std::move(availableDevicesCb))
{
}

ErrCode MockHelperModuleImpl::getName(IString** name)
{
    return createString(name, "MockHelperModule");
}

ErrCode MockHelperModuleImpl::getId(IString** id)
{
    return createString(id, "MockHelperModule");
}

ErrCode MockHelperModuleImpl::getAvailableDevices(IList** availableDevices)
{
    ListPtr<IDeviceInfo> availableDevicesPtr;
    if (availableDevicesCb.assigned())
    {
        ErrCode errCode = wrapHandlerReturn(availableDevicesCb, availableDevicesPtr);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }
    else
    {
        availableDevicesPtr = List<IDeviceInfo>();
    }

    *availableDevices = availableDevicesPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockHelperModuleImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    auto types = Dict<IString, IDeviceType>();
    *deviceTypes = types.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockHelperModuleImpl::createDevice(IDevice** /*device*/,
                                           IString* /*connectionString*/,
                                           IComponent* /*parent*/,
                                           IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode MockHelperModuleImpl::getAvailableFunctionBlockTypes(IDict** types)
{
    auto list = Dict<IString, IFunctionBlockType>();
    *types = list.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockHelperModuleImpl::createFunctionBlock(IFunctionBlock** /*functionBlock*/,
                                                  IString* /*id*/,
                                                  IComponent* /*parent*/,
                                                  IString* /*localId*/,
                                                  IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode MockHelperModuleImpl::getAvailableServerTypes(IDict** serverTypes)
{
    *serverTypes = Dict<IString, IServerType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockHelperModuleImpl::createServer(IServer** /*server*/,
                                           IString* /*serverType*/,
                                           IDevice* /*rootDevice*/,
                                           IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockHelperModuleImpl::getVersionInfo(IVersionInfo** version)
{
    OPENDAQ_PARAM_NOT_NULL(version);

    *version = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockHelperModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                              IString* /*connectionString*/,
                                              IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockHelperModuleImpl::completeServerCapability(Bool* /*succeeded*/, IServerCapability* /*source*/, IServerCapabilityConfig* /*target*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockHelperModuleImpl::getAvailableStreamingTypes(IDict** /*streamingTypes*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

}

END_NAMESPACE_OPENDAQ
