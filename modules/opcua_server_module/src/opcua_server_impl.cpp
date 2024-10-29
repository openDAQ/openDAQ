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

OpcUaServerImpl::OpcUaServerImpl(const DevicePtr& rootDevice,
                                 const PropertyObjectPtr& config,
                                 const ContextPtr& context)
    : Server("OpenDAQOPCUA", config, rootDevice, context)
    , server(rootDevice, context)
    , context(context)
{
    const uint16_t port = config.getPropertyValue("Port");

    server.setOpcUaPort(port);
    server.setOpcUaPath(config.getPropertyValue("Path"));
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

    auto options = context.getModuleOptions("OpenDAQOPCUAServerModule");
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

PropertyObjectPtr OpcUaServerImpl::populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context)
{
    const auto defConfig = createDefaultConfig(context);
    for (const auto& prop : defConfig.getAllProperties())
    {
        const auto name = prop.getName();
        if (config.hasProperty(name))
            defConfig.setPropertyValue(name, config.getPropertyValue(name));
    }

    return defConfig;
}

PropertyObjectPtr OpcUaServerImpl::getDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_opcua-tcp._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "OPENDAQ"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("Port")));
    discoveryConfig.addProperty(StringProperty("ProtocolVersion", ""));
    return discoveryConfig;
}

ServerTypePtr OpcUaServerImpl::createType(const ContextPtr& context)
{
    return ServerType("OpenDAQOPCUA",
                      "openDAQ OPC UA server",
                      "Publishes device structure over OPC UA protocol",
                      OpcUaServerImpl::createDefaultConfig(context));
}

void OpcUaServerImpl::onStopServer()
{
    server.stop();
    if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr; rootDevice.assigned())
    {
        const auto info = rootDevice.getInfo();
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();
        if (info.hasServerCapability("OpenDAQOPCUAConfiguration"))
            infoInternal.removeServerCapability("OpenDAQOPCUAConfiguration");
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, OpcUaServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE
