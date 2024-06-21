#include <opcua_server_module/opcua_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE
using namespace daq;
using namespace daq::opcua;

OpcUaServerImpl::OpcUaServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server("OpcUaServer", config, rootDevice, context, nullptr)
    , server(rootDevice, context)
    , context(context)
{
    const uint16_t port = config.getPropertyValue("Port");

    server.setOpcUaPort(port);
    server.start();
}

OpcUaServerImpl::~OpcUaServerImpl()
{
}

void OpcUaServerImpl::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
{
    if (!context.assigned())
        return;
    if (!config.assigned())
        return;

    auto options = context.getModuleOptions("OpcUaServer");
    for (const auto& [key, value] : options)
    {
        if (config.hasProperty(key))
        {
            config->setPropertyValue(key, value);
        }
    }
}

PropertyObjectPtr OpcUaServerImpl::createDefaultConfig(const ContextPtr& context)
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto portProp = IntPropertyBuilder("Port", 4840)
        .setMinValue(minPortValue)
        .setMaxValue(maxPortValue)
        .build();
    defaultConfig.addProperty(portProp);

    defaultConfig.addProperty(StringProperty("Path", "/"));

    populateDefaultConfigFromProvider(context, defaultConfig);
    return defaultConfig;
}

PropertyObjectPtr OpcUaServerImpl::getDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_opcua-tcp._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "OPENDAQ"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("Port")));
    return discoveryConfig;
}

ServerTypePtr OpcUaServerImpl::createType(const ContextPtr& context)
{
    return ServerType("openDAQ OpcUa",
                      "openDAQ OpcUa server",
                      "Publishes device structure over OpcUa protocol",
                      OpcUaServerImpl::createDefaultConfig(context));
}

void OpcUaServerImpl::onStopServer()
{
    server.stop();
    if (this->rootDevice.assigned())
    {
        const auto info = this->rootDevice.getInfo();
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();
        if (info.hasServerCapability("opendaq_opcua_config"))
            infoInternal.removeServerCapability("opendaq_opcua_config");
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, OpcUaServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE
