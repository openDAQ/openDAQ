#include <testutils/testutils.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_internal_ptr.h>
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
                                    .setTransportOptions(List<ITransportOption>(TransportOption("plain", 7420),
                                                                                TransportOption("tls", 7421, TransportProtection::Encrypted)))
                                    .build();
        deviceTypes.set("dev", deviceType);

        const auto streamingType = StreamingTypeBuilder()
                                       .setId("str")
                                       .setName("str")
                                       .setConnectionStringPrefix("str")
                                       .setTransportOptions(List<ITransportOption>(TransportOption("plain", 7430),
                                                                                   TransportOption("tls", 7431, TransportProtection::Encrypted)))
                                       .build();
        streamingTypes.set("str", streamingType);
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

    DictPtr<IString, IDeviceType> deviceTypes = Dict<IString, IDeviceType>();
    DictPtr<IString, IStreamingType> streamingTypes = Dict<IString, IStreamingType>();
    ListPtr<IDeviceInfo> availableDevices = List<IDeviceInfo>();
    ListPtr<IString> connectionStrings = List<IString>();
    ListPtr<IPropertyObject> typeConfigs = List<IPropertyObject>();
};

ServerCapabilityConfigPtr Capability(const StringPtr& id, ProtocolType type, const StringPtr& prefix, const ListPtr<ITransportOption>& options)
{
    const Int port = options[0].getPort();
    auto capability =
        ServerCapability(id, id, type).setPrefix(prefix).setConnectionString(prefix.toStdString() + "://host:" + std::to_string(port));
    capability.setTransportOptions(options);
    return capability;
}

// A device as discovery reports it: its server serves plain and TLS on ports of its own, plain first
DeviceInfoConfigPtr DiscoveredDeviceInfo()
{
    auto info = DeviceInfo("daq://m_s");
    info.setManufacturer("m");
    info.setSerialNumber("s");

    const auto internal = info.asPtr<IDeviceInfoInternal>();
    internal.addServerCapability(Capability("devproto",
                                            ProtocolType::Configuration,
                                            "dev",
                                            List<ITransportOption>(TransportOption("plain", 17420),
                                                                   TransportOption("tls", 17422, TransportProtection::Encrypted))));
    internal.addServerCapability(Capability("strproto",
                                            ProtocolType::Streaming,
                                            "str",
                                            List<ITransportOption>(TransportOption("plain", 17430),
                                                                   TransportOption("tls", 17432, TransportProtection::Encrypted))));
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

    ErrCode create(const StringPtr& connectionString, const PropertyObjectPtr& config, DevicePtr& device)
    {
        const ErrCode errCode = utils()->createDevice(&device, connectionString, parent, config);
        daqClearErrorInfo();
        return errCode;
    }

    ErrCode create(const StringPtr& connectionString, const PropertyObjectPtr& config)
    {
        DevicePtr device;
        return create(connectionString, config, device);
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

// The default add-device config selects the transport option by name

TEST_F(TransportSelectionTest, UnknownTransportOptionRejected)
{
    const PropertyObjectPtr config = utils().createDefaultAddDeviceConfig();
    config.setPropertyValue("Device.dev.TransportOption", "wss");

    ASSERT_EQ(create("dev://host", config), OPENDAQ_ERR_NOTFOUND);
    ASSERT_EQ(module->connectionStrings.getCount(), 0u);
}
