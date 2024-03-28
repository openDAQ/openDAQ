#include "mock_module.h"

#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/validation.h>
#include <coretypes/dictobject_factory.h>

using namespace daq;

ErrCode MockModuleImpl::getName(IString** name)
{
    return createString(name, "MockModule");
}

ErrCode MockModuleImpl::getId(IString** id)
{
    return createString(id, "Mock");
}

ErrCode MockModuleImpl::getAvailableDevices(IList** availableDevices)
{
    OPENDAQ_PARAM_NOT_NULL(availableDevices);

    *availableDevices = List<IDeviceInfo>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    *deviceTypes = Dict<IString, IDeviceType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::acceptsConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(accepted);

    *accepted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config)
{
    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode MockModuleImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);

    *functionBlockTypes = Dict<IString, IFunctionBlockType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, daq::IString* localId, daq::IPropertyObject* config)
{
    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode MockModuleImpl::getAvailableServerTypes(IDict** serverTypes)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockModuleImpl::createServer(daq::IServer** server, daq::IString* serverType, daq::IDevice* rootDevice, daq::IPropertyObject* config)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockModuleImpl::getVersionInfo(IVersionInfo** version)
{
    if (version == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *version = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::acceptsStreamingConnectionParameters(Bool* accepted,
                                                             IString* /*connectionString*/,
                                                             daq::IServerCapability* /*capability*/)
{
    OPENDAQ_PARAM_NOT_NULL(accepted);

    *accepted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                        IString* /*connectionString*/,
                                        daq::IServerCapability* /*capability*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockModule, IModule)
