#include <opcua_server_module/opcua_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE

using namespace daq;
using namespace daq::opcua;

OpcUaServerImpl::OpcUaServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server(nullptr, rootDevice, nullptr, nullptr)
    , server(rootDevice, context)
    , config(config)
    , context(context)
{
    const uint16_t port = config.getPropertyValue("Port");

    server.setOpendaqVersion(OPENDAQ_PACKAGE_VERSION);
    server.setOpcUaPort(port);
    server.start();
}

PropertyObjectPtr OpcUaServerImpl::createDefaultConfig()
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto portProp = IntPropertyBuilder("Port", 4840).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(portProp);

    return defaultConfig;
}

ServerTypePtr OpcUaServerImpl::createType()
{
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&OpcUaServerImpl::createDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return ServerType("openDAQ OpcUa",
                      "openDAQ OpcUa server",
                      "Publishes device structure over OpcUa protocol",
                      configurationCallback);
}

void OpcUaServerImpl::onStopServer()
{
    server.stop();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, OpcUaServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE
