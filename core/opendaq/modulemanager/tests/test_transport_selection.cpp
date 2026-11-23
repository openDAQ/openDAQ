#include <testutils/testutils.h>
#include <testutils/transport_options.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include "mock/mock_module.h"

using namespace daq;

namespace
{
class TransportTestDevice : public Device
{
public:
    TransportTestDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
        , info(DeviceInfo("dev://"))
    {
    }

protected:
    DeviceInfoPtr onGetInfo() override
    {
        return info;
    }

private:
    DeviceInfoConfigPtr info;
};

// Lists transport options on its types and records the devices it is asked to create
class TransportTestModule : public MockModuleImpl
{
public:
    TransportTestModule()
        : MockModuleImpl(String("transport_test"))
    {
        const auto deviceType = DeviceTypeBuilder()
                                    .setId("dev")
                                    .setName("dev")
                                    .setConnectionStringPrefix("dev")
                                    .setDeviceKind(DeviceKind::Mirrored)
                                    .setAvailableTransportOptions(TransportOptionsById(
                                        {TransportOption("plain", 7420), TransportOption("tls", 7421, SecurityMode::Encrypted)}))
                                    .build();
        deviceTypes.set("dev", deviceType);

        const auto streamingType = StreamingTypeBuilder()
                                       .setId("str")
                                       .setName("str")
                                       .setConnectionStringPrefix("str")
                                       .setAvailableTransportOptions(TransportOptionsById(
                                           {TransportOption("plain", 7430), TransportOption("tls", 7431, SecurityMode::Encrypted)}))
                                       .build();
        streamingTypes.set("str", streamingType);

        // A type of a module that does not use transport options
        deviceTypes.set("legacy", DeviceTypeBuilder().setId("legacy").setName("legacy").setConnectionStringPrefix("legacy").build());
    }

    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** types) override
    {
        *types = deviceTypes.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** types) override
    {
        *types = streamingTypes.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** devices) override
    {
        *devices = availableDevices.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* /*parent*/, IPropertyObject* config) override
    {
        connectionStrings.pushBack(connectionString);
        typeConfigs.pushBack(config);

        // A context of its own keeps the device out of the module manager's ownership cycle
        *device = createWithImplementation<IDevice, TransportTestDevice>(NullContext(), nullptr, String("dev")).detach();
        return OPENDAQ_SUCCESS;
    }

    // Records the config and fails: the tests only look at what the module is given
    ErrCode INTERFACE_FUNC createStreaming(IStreaming** /*streaming*/, IString* /*connectionString*/, IPropertyObject* config) override
    {
        streamingTypeConfigs.pushBack(config);
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "No streaming in the transport tests");
    }

    DictPtr<IString, IDeviceType> deviceTypes = Dict<IString, IDeviceType>();
    DictPtr<IString, IStreamingType> streamingTypes = Dict<IString, IStreamingType>();
    ListPtr<IDeviceInfo> availableDevices = List<IDeviceInfo>();
    ListPtr<IString> connectionStrings = List<IString>();
    ListPtr<IPropertyObject> typeConfigs = List<IPropertyObject>();
    ListPtr<IPropertyObject> streamingTypeConfigs = List<IPropertyObject>();
};

ServerCapabilityConfigPtr Capability(const StringPtr& id,
                                     ProtocolType type,
                                     const StringPtr& prefix,
                                     const DictPtr<IString, ITransportOption>& options)
{
    const Int port = options.getValueList()[0].getPort();
    auto capability =
        ServerCapability(id, id, type).setPrefix(prefix).setConnectionString(prefix.toStdString() + "://host:" + std::to_string(port));
    capability.setAvailableTransportOptions(options);
    return capability;
}

// A device as discovery reports it: its server serves plain and TLS on ports of its own, plain first
DeviceInfoConfigPtr DiscoveredDeviceInfo()
{
    auto info = DeviceInfo("daq://m_s");
    info.setManufacturer("m");
    info.setSerialNumber("s");

    const auto internal = info.asPtr<IDeviceInfoInternal>();
    internal.addServerCapability(
        Capability("devproto",
                   ProtocolType::Configuration,
                   "dev",
                   TransportOptionsById({TransportOption("plain", 17420), TransportOption("tls", 17422, SecurityMode::Encrypted)})));
    internal.addServerCapability(
        Capability("strproto",
                   ProtocolType::Streaming,
                   "str",
                   TransportOptionsById({TransportOption("plain", 17430), TransportOption("tls", 17432, SecurityMode::Encrypted)})));
    return info;
}
}

class TransportSelectionTest : public testing::Test
{
protected:
    TransportSelectionTest()
        : manager(ModuleManager("[[none]]"))
        , parent(Component(NullContext(), nullptr, "parent"))
    {
        manager.loadModules(NullContext());

        module = new TransportTestModule();
        IModule* intf = module;
        modulePtr = ModulePtr(intf);
        module->availableDevices.pushBack(DiscoveredDeviceInfo());
        checkErrorInfo(manager->addModule(modulePtr));
    }

    ErrCode create(const StringPtr& connectionString, const PropertyObjectPtr& config)
    {
        DevicePtr device;
        const ErrCode errCode = utils()->createDevice(&device, connectionString, parent, config);
        daqClearErrorInfo();
        return errCode;
    }

    ModuleManagerUtilsPtr utils() const
    {
        return manager.asPtr<IModuleManagerUtils>();
    }

    PropertyObjectPtr lastTypeConfig() const
    {
        return module->typeConfigs[module->typeConfigs.getCount() - 1];
    }

    ModuleManagerPtr manager;
    ComponentPtr parent;
    TransportTestModule* module = nullptr;
    ModulePtr modulePtr;
};

// The module gets the selection properties right before it creates a device or streaming

TEST_F(TransportSelectionTest, DeviceGetsSelectionOfDefaultOption)
{
    ASSERT_EQ(create("dev://host", nullptr), OPENDAQ_SUCCESS);
    ASSERT_EQ(lastTypeConfig().getPropertyValue(TransportOptionConfigProperty), "plain");
    ASSERT_EQ(lastTypeConfig().getPropertyValue(PortConfigProperty), 7420);

    // A config of the type itself carries no selection
    const DeviceTypePtr type = module->deviceTypes.get("dev");
    ASSERT_EQ(create("dev://host", type.createDefaultConfig()), OPENDAQ_SUCCESS);
    ASSERT_EQ(lastTypeConfig().getPropertyValue(TransportOptionConfigProperty), "plain");
}

TEST_F(TransportSelectionTest, StreamingGetsSelectionOfDefaultOption)
{
    const StreamingTypePtr type = module->streamingTypes.get("str");
    ASSERT_ANY_THROW(utils().createStreaming("str://host", type.createDefaultConfig()));
    ASSERT_ANY_THROW(utils().createStreaming("str://host", nullptr));

    ASSERT_EQ(module->streamingTypeConfigs.getCount(), 2u);
    for (const PropertyObjectPtr& config : module->streamingTypeConfigs)
    {
        ASSERT_EQ(config.getPropertyValue(TransportOptionConfigProperty), "plain");
        ASSERT_EQ(config.getPropertyValue(PortConfigProperty), 7430);
    }
}

// A type config that holds the id of an option the type does not list is rejected. The streaming shows it: the type
// config of a device is built from the type's default config, which drops a TransportOption the type does not declare.
TEST_F(TransportSelectionTest, UnknownTransportOptionRejected)
{
    const StreamingTypePtr type = module->streamingTypes.get("str");
    const PropertyObjectPtr config = type.createDefaultConfig();
    config.addProperty(StringProperty(TransportOptionConfigProperty, "wss"));

    ASSERT_THROW(utils().createStreaming("str://host", config), NotFoundException);
    ASSERT_EQ(module->streamingTypeConfigs.getCount(), 0u);
}

// The add-device config selects among the options of a device info

TEST_F(TransportSelectionTest, ConfigListsServedOptions)
{
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(DiscoveredDeviceInfo());

    ASSERT_EQ(config.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    ASSERT_EQ(config.getPort(), 17420);

    const auto streamingConfig = config.getStreamingOptions().getStreamingConfig();
    ASSERT_EQ(streamingConfig.getStreamingType().getId(), "str");
    ASSERT_EQ(streamingConfig.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    ASSERT_EQ(streamingConfig.getPort(), 17430);
}

// A device as discovery reports it, with only a configuration capability announcing the given options
static DeviceInfoConfigPtr DeviceInfoAnnouncing(const DictPtr<IString, ITransportOption>& options)
{
    auto info = DeviceInfo("daq://m_s");
    info.setManufacturer("m");
    info.setSerialNumber("s");
    info.asPtr<IDeviceInfoInternal>().addServerCapability(Capability("devproto", ProtocolType::Configuration, "dev", options));
    return info;
}

// The config lists the announced options the type knows, in the server's order with its ports and the type's attributes
TEST_F(TransportSelectionTest, ConfigListsOnlyOptionsTheTypeKnows)
{
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(DeviceInfoAnnouncing(TransportOptionsById(
        {TransportOption("quic", 17425), TransportOption("tls", 17422), TransportOption("plain", 17420, SecurityMode::Signed, True)})));

    const auto options = config.getAvailableTransportOptions();
    ASSERT_EQ(options.getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(options.get("tls").getPort(), 17422);
    ASSERT_EQ(options.get("tls").getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_EQ(options.get("plain").getPort(), 17420);
    ASSERT_EQ(options.get("plain").getSecurityMode(), SecurityMode::None);
    ASSERT_FALSE(options.get("plain").getCertificateRequired());

    ASSERT_EQ(config.getSelectedTransportOption().getId(), "tls");
    ASSERT_EQ(config.getPort(), 17422);
}

// A server that announces only options the type does not know leaves the config without options, also once serialized
TEST_F(TransportSelectionTest, ConfigWithoutOptionsTheTypeKnows)
{
    const auto unknownOptions = TransportOptionsById({TransportOption("quic", 17425), TransportOption("wss", 17443)});
    const auto info = DeviceInfoAnnouncing(unknownOptions);
    info.asPtr<IDeviceInfoInternal>().addServerCapability(Capability("strproto", ProtocolType::Streaming, "str", unknownOptions));
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(info);

    ASSERT_EQ(config.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_FALSE(config.getSelectedTransportOption().assigned());
    ASSERT_EQ(config.getPort(), -1);

    const auto serializer = JsonSerializer();
    config.serialize(serializer);
    const AddDeviceConfigPtr deserialized = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);

    ASSERT_EQ(deserialized.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_FALSE(deserialized.getSelectedTransportOption().assigned());
    ASSERT_EQ(deserialized.getPort(), -1);

    const auto streamingConfig = deserialized.getStreamingOptions().getAvailableStreamingConfigs().get("str");
    ASSERT_EQ(streamingConfig.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_FALSE(streamingConfig.getSelectedTransportOption().assigned());
}

// The config keeps the options the server announces; only a saved config selects among the type's again
TEST_F(TransportSelectionTest, ServedOptionsSerialized)
{
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(DiscoveredDeviceInfo());
    config.setTransportOption("tls");

    const auto serializer = JsonSerializer();
    config.serialize(serializer);
    const AddDeviceConfigPtr deserialized = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);

    ASSERT_EQ(deserialized.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    ASSERT_EQ(deserialized.getSelectedTransportOption().getId(), "tls");
    ASSERT_EQ(deserialized.getPort(), 17422);

    const auto streamingConfig = deserialized.getStreamingOptions().getAvailableStreamingConfigs().get("str");
    ASSERT_EQ(streamingConfig.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    ASSERT_EQ(streamingConfig.getPort(), 17430);
}

TEST_F(TransportSelectionTest, ServedOptionsCloned)
{
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(DiscoveredDeviceInfo());
    config.setTransportOption("tls");

    const auto cloned = config.asPtr<IPropertyObjectInternal>().clone().asPtr<IAddDeviceConfig, AddDeviceConfigPtr>();
    ASSERT_EQ(cloned.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    ASSERT_EQ(cloned.getPort(), 17422);
    ASSERT_EQ(cloned.getStreamingOptions().getAvailableStreamingConfigs().get("str").getPort(), 17430);
}

TEST_F(TransportSelectionTest, HandMadeDeviceInfoListsTypeOptions)
{
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(DeviceInfo("dev://host"));
    ASSERT_EQ(config.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    ASSERT_EQ(config.getPort(), 7420);
    ASSERT_EQ(config.getStreamingOptions().getStreamingConfig().getPort(), 7430);

    // The port the connection string of the device info names is not taken: the port follows the selected option
    const AddDeviceConfigPtr withPort = utils().createAddDeviceConfig(DeviceInfo("dev://host:9000"));
    ASSERT_EQ(withPort.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(withPort.getPort(), 7420);
    withPort.setTransportOption("tls");
    ASSERT_EQ(withPort.getPort(), 7421);
}

// A Device type without options has no port until one is set, whatever the connection string of the device info names
TEST_F(TransportSelectionTest, HandMadeDeviceInfoWithoutTypeOptions)
{
    const AddDeviceConfigPtr config = utils().createAddDeviceConfig(DeviceInfo("legacy://host:9000"));
    ASSERT_EQ(config.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_FALSE(config.getSelectedTransportOption().assigned());
    ASSERT_EQ(config.getPort(), -1);
}
