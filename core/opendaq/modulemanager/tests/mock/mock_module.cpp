#include "mock_module.h"

#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/validation.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/module_info_factory.h>

using namespace daq;

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

ErrCode MockModuleImpl::createServer(daq::IServer** server,
                                     daq::IString* serverType,
                                     daq::IDevice* rootDevice,
                                     daq::IPropertyObject* config)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockModuleImpl::getModuleInfo(IModuleInfo** info)
{
    if (info == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *info = ModuleInfo(VersionInfo(0, 0, 0), "MockModule", "mock").detach();

    return OPENDAQ_SUCCESS;
}

ErrCode MockModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                        IString* /*connectionString*/,
                                        IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockModuleImpl::completeServerCapability(daq::Bool* /*succeeded*/, daq::IServerCapability* /*source*/, daq::IServerCapabilityConfig* /*target*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

daq::ErrCode MockModuleImpl::getAvailableStreamingTypes(daq::IDict** streamingTypes)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockModule, IModule)
