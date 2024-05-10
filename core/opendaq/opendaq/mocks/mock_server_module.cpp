#include "opendaq/mock/mock_server_module.h"
#include "opendaq/mock/mock_server.h"
#include <coretypes/impl.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/server_type_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/mock/mock_server_factory.h>

using namespace daq;

MockServerModuleImpl::MockServerModuleImpl(daq::ContextPtr ctx, IModuleManager* mng)
    : ctx(std::move(ctx))
    , manager(mng)
{
}

ErrCode MockServerModuleImpl::getName(IString** name)
{
    return createString(name, "MockServerModule");
}

ErrCode MockServerModuleImpl::getId(IString** id)
{
    return createString(id, "MockServer");
}

ErrCode MockServerModuleImpl::getAvailableDevices(IList** availableDevices)
{
    *availableDevices = List<IDeviceInfo>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    *deviceTypes = Dict<IString, IDeviceType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::acceptsConnectionParameters(Bool* accepted, IString* /*connectionString*/, IPropertyObject* /*config*/)
{
    *accepted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::createDevice(IDevice** device,
                                           IString* /*connectionString*/,
                                           IComponent* /*parent*/,
                                           IPropertyObject* /*config*/)
{
    *device = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    *functionBlockTypes = Dict<IString, IFunctionBlockType>().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* /*id*/, daq::IComponent* /*parent*/, IString* /*localId*/, IPropertyObject* /*config*/)
{
    *functionBlock = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::getAvailableServerTypes(IDict** serverTypes)
{
    auto types = Dict<IString, IServerType>();

    types.set("MockServer", ServerType("MockServer", "Mock Server", "Mock"));
    types.set("openDAQ LT Streaming", ServerType("openDAQ LT Streaming", "Mock Server", "Mock"));
    types.set("openDAQ Native Streaming", ServerType("openDAQ Native Streaming", "Mock Server", "Mock"));
    types.set("openDAQ OpcUa", ServerType("openDAQ OpcUa", "Mock Server", "Mock"));

    *serverTypes = types.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::createServer(IServer** server,
                                           IString* serverType,
                                           IDevice* /*rootDevice*/,
                                           IPropertyObject* /*config*/
                                           )
{
    const StringPtr serverTypePtr = StringPtr::Borrow(serverType);
    if (serverTypePtr == "MockServer" ||
        serverTypePtr == "openDAQ LT Streaming" ||
        serverTypePtr == "openDAQ Native Streaming" ||
        serverTypePtr == "openDAQ OpcUa")
    {
        *server = MockServer().detach();
    }
    else
    {
        return OPENDAQ_ERR_INVALIDPARAMETER;
    }
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::getVersionInfo(IVersionInfo** version)
{
    if (version == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *version = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::acceptsStreamingConnectionParameters(Bool* accepted,
                                                                   IString* /*connectionString*/,
                                                                   IPropertyObject* /*config*/)
{
    OPENDAQ_PARAM_NOT_NULL(accepted);

    *accepted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerModuleImpl::createStreaming(IStreaming** /*streaming*/,
                                              IString* /*connectionString*/,
                                              IPropertyObject* /*config*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockServerModuleImpl::createConnectionString(IString** /*connectionString*/, IServerCapability* /*serverCapability*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MockServerModuleImpl::getAvailableStreamingTypes(daq::IDict** streamingTypes)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockServerModule,
    IModule,
    IContext*,
    ctx,
    IModuleManager*,
    manager
    )
