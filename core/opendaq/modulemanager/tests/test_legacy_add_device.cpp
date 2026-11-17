#include <testutils/testutils.h>
#include "mock/mock_module.h"

#include <opendaq/address_info_factory.h>
#include <opendaq/client_type.h>
#include <opendaq/component_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_impl.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/logger_factory.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/server_capability_config_ptr.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>

#include <functional>
#include <map>

using namespace daq;

namespace
{

using Values = std::map<std::string, std::string>;

void collect(const PropertyObjectPtr& obj, const std::string& prefix, Values& values)
{
    for (const auto& prop : obj.getAllProperties())
    {
        const std::string name = prop.getName();
        const auto value = obj.getPropertyValue(name);
        if (prop.getValueType() == ctObject)
            collect(value, prefix + name + ".", values);
        else
            values[prefix + name] = value.assigned() ? static_cast<std::string>(value.toString()) : "<null>";
    }
}

Values snapshot(const PropertyObjectPtr& obj)
{
    Values values;
    if (obj.assigned())
        collect(obj, "", values);
    return values;
}

PropertyObjectPtr legacyTypeConfig()
{
    auto nested = PropertyObject();
    nested.addProperty(IntProperty("Value", 1));

    auto config = PropertyObject();
    config.addProperty(IntProperty("Port", 7420));
    config.addProperty(StringProperty("Host", ""));
    config.addProperty(StringProperty("Username", ""));
    config.addProperty(StringProperty("Password", ""));
    ClientTypeTools::DefineConfigProperties(config);
    config.addProperty(ObjectProperty("Nested", nested));
    return config;
}

class LegacyTestDevice : public Device
{
public:
    LegacyTestDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
        , info(DeviceInfo("legdev://dev"))
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

class LegacyTestModule : public MockModuleImpl
{
public:
    LegacyTestModule()
        : MockModuleImpl(String("legacy_test_module"))
    {
        deviceType = DeviceTypeBuilder()
                         .setId("legdev")
                         .setName("legdev")
                         .setConnectionStringPrefix("legdev")
                         .setDefaultConfig(legacyTypeConfig())
                         .build();

        auto streamingConfig = PropertyObject();
        streamingConfig.addProperty(IntProperty("Port", 7414));
        streamingConfig.addProperty(StringProperty("Username", ""));
        streamingType = StreamingTypeBuilder()
                            .setId("legstr")
                            .setName("legstr")
                            .setConnectionStringPrefix("legstr")
                            .setDefaultConfig(streamingConfig)
                            .build();
    }

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override
    {
        scanCount++;
        *availableDevices = List<IDeviceInfo>(discover()).detach();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override
    {
        *deviceTypes = Dict<IString, IDeviceType>({{deviceType.getId(), deviceType}}).detach();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) override
    {
        *streamingTypes = Dict<IString, IStreamingType>({{streamingType.getId(), streamingType}}).detach();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC createDevice(IDevice** device,
                                        IString* connectionString,
                                        IComponent* /*parent*/,
                                        IPropertyObject* config) override
    {
        connectionStrings.push_back(StringPtr(connectionString).toStdString());
        configs.push_back(snapshot(PropertyObjectPtr(config)));
        *device = createWithImplementation<IDevice, LegacyTestDevice>(NullContext(), nullptr, String("dev")).detach();
        return OPENDAQ_SUCCESS;
    }

    DeviceTypePtr deviceType;
    StreamingTypePtr streamingType;
    std::function<DeviceInfoPtr()> discover = [] { return DeviceInfo("legdev://unused"); };
    int scanCount = 0;
    std::vector<std::string> connectionStrings;
    std::vector<Values> configs;
};

}

class LegacyAddDeviceTest : public testing::Test
{
protected:
    void SetUp() override
    {
        setUpManager(nullptr);
    }

    void setUpManager(const DictPtr<IString, IBaseObject>& options)
    {
        manager = ModuleManager("[[none]]");
        context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);
        auto moduleObj = createWithImplementation<IModule, LegacyTestModule>();
        module = reinterpret_cast<LegacyTestModule*>(moduleObj.getObject());
        manager.addModule(moduleObj);
        utils = manager.asPtr<IModuleManagerUtils>();
    }

    struct Address
    {
        std::string type;
        std::string name = type;
    };

    void addDiscoveredDevice(const std::vector<Address>& addresses)
    {
        module->discover = [addresses]
        {
            auto info = DeviceInfo("legdev://unused");
            info.setManufacturer("m");
            info.setSerialNumber("s");
            auto capability = ServerCapability("legdev_protocol", "legdev_protocol", ProtocolType::Configuration)
                                  .setPrefix("legdev")
                                  .setConnectionString("legdev://capability");
            for (const auto& address : addresses)
            {
                capability.addAddressInfo(AddressInfoBuilder()
                                              .setAddress(address.name)
                                              .setConnectionString("legdev://" + address.name)
                                              .setType(address.type)
                                              .build());
            }
            info.asPtr<IDeviceInfoInternal>().addServerCapability(capability);
            return info;
        };
    }

    PropertyObjectPtr legacyConfig()
    {
        return utils.createDefaultAddDeviceConfig();
    }

    static PropertyObjectPtr section(const PropertyObjectPtr& config, const std::string& path)
    {
        return config.getPropertyValue(path);
    }

    void add(const std::string& connectionString, const PropertyObjectPtr& config)
    {
        utils.createDevice(connectionString, parent, config);
    }

    const Values& lastConfig() const
    {
        return module->configs.back();
    }

    const std::string& lastConnectionString() const
    {
        return module->connectionStrings.back();
    }

    static Values defaults()
    {
        return {
            {"Port", "7420"},
            {"Host", ""},
            {"Username", ""},
            {"Password", ""},
            {"ClientType", "0"},
            {"ExclusiveControlDropOthers", "False"},
            {"Nested.Value", "1"},
        };
    }

    static Values with(Values values, const Values& changes)
    {
        for (const auto& [name, value] : changes)
            values[name] = value;
        return values;
    }

    ModuleManagerPtr manager;
    ContextPtr context;
    ModuleManagerUtilsPtr utils;
    LegacyTestModule* module = nullptr;
    ComponentPtr parent = Component(NullContext(), nullptr, "parent");
};

TEST_F(LegacyAddDeviceTest, NullConfig)
{
    add("legdev://host", nullptr);

    ASSERT_EQ(lastConnectionString(), "legdev://host");
    ASSERT_EQ(lastConfig(), defaults());
    ASSERT_EQ(module->scanCount, 1);
}

TEST_F(LegacyAddDeviceTest, DefaultConfig)
{
    add("legdev://host", legacyConfig());

    ASSERT_EQ(lastConnectionString(), "legdev://host");
    ASSERT_EQ(lastConfig(), defaults());
    ASSERT_EQ(module->scanCount, 1);
}

TEST_F(LegacyAddDeviceTest, DeviceSection)
{
    const auto config = legacyConfig();
    const auto typeConfig = section(config, "Device.legdev");
    typeConfig.setPropertyValue("Port", 1);
    typeConfig.setPropertyValue("Host", "h");
    typeConfig.setPropertyValue("Nested.Value", 5);

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Port", "1"}, {"Host", "h"}, {"Nested.Value", "5"}}));
}

TEST_F(LegacyAddDeviceTest, GeneralCredentialsFillTypeConfig)
{
    const auto config = legacyConfig();
    const auto general = section(config, "General");
    general.setPropertyValue("Username", "u");
    general.setPropertyValue("Password", "p");
    general.setPropertyValue("ClientType", 2);
    general.setPropertyValue("ExclusiveControlDropOthers", true);

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(),
              with(defaults(), {{"Username", "u"}, {"Password", "p"}, {"ClientType", "2"}, {"ExclusiveControlDropOthers", "True"}}));
}

TEST_F(LegacyAddDeviceTest, GeneralCredentialsKeepChangedTypeConfig)
{
    const auto config = legacyConfig();
    const auto general = section(config, "General");
    general.setPropertyValue("Username", "general");
    general.setPropertyValue("Password", "general");
    const auto typeConfig = section(config, "Device.legdev");
    typeConfig.setPropertyValue("Username", "device");

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Username", "device"}, {"Password", "general"}}));
}

TEST_F(LegacyAddDeviceTest, GeneralCredentialsOverrideTypeConfigSetToDefault)
{
    const auto config = legacyConfig();
    section(config, "General").setPropertyValue("Username", "general");
    section(config, "Device.legdev").setPropertyValue("Username", "");

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Username", "general"}}));
}

TEST_F(LegacyAddDeviceTest, FlatConfig)
{
    auto config = PropertyObject();
    config.addProperty(IntProperty("Port", 5));
    config.addProperty(StringProperty("Username", "u"));
    config.addProperty(IntProperty("Unknown", 1));

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Port", "5"}, {"Username", "u"}}));
}

TEST_F(LegacyAddDeviceTest, FlatConfigGeneralKey)
{
    auto config = PropertyObject();
    config.addProperty(StringProperty("Password", "p"));
    config.addProperty(BoolProperty("ScanOnAdd", false));

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Password", "p"}}));
    ASSERT_EQ(module->scanCount, 0);
}

TEST_F(LegacyAddDeviceTest, BareTypeConfig)
{
    const PropertyObjectPtr config = module->deviceType.createDefaultConfig();
    config.setPropertyValue("Port", 6);
    config.setPropertyValue("Nested.Value", 7);

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Port", "6"}, {"Nested.Value", "7"}}));
}

TEST_F(LegacyAddDeviceTest, ConnectionStringOptionsOverrideConfig)
{
    const auto config = legacyConfig();
    section(config, "Device.legdev").setPropertyValue("Port", 1);

    add("legdev://host?Port=9", config);

    ASSERT_EQ(lastConnectionString(), "legdev://host");
    ASSERT_EQ(lastConfig(), with(defaults(), {{"Port", "9"}}));
}

TEST_F(LegacyAddDeviceTest, ConnectionStringOptionsOverrideFlatConfig)
{
    auto config = PropertyObject();
    config.addProperty(IntProperty("Port", 5));

    add("legdev://host?Port=9", config);

    ASSERT_EQ(lastConnectionString(), "legdev://host");
    ASSERT_EQ(lastConfig(), with(defaults(), {{"Port", "9"}}));
}

TEST_F(LegacyAddDeviceTest, GeneralScanOnAdd)
{
    setUpManager(Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 0}})}}));
    addDiscoveredDevice({{"IPv4"}});

    const auto config = legacyConfig();
    section(config, "General").setPropertyValue("ScanOnAdd", false);
    add("legdev://host", config);
    ASSERT_EQ(module->scanCount, 0);

    section(config, "General").setPropertyValue("ScanOnAdd", true);
    add("legdev://host", config);
    ASSERT_EQ(module->scanCount, 1);

    section(config, "General").setPropertyValue("ScanOnAdd", false);
    add("daq://m_s", config);
    ASSERT_EQ(module->scanCount, 1);

    section(config, "General").setPropertyValue("ScanOnAdd", true);
    add("daq://m_s", config);
    ASSERT_EQ(module->scanCount, 2);
}

TEST_F(LegacyAddDeviceTest, GeneralScanOnAddOverridesManagerOption)
{
    const auto options = Dict<IString, IBaseObject>({{"ScanOnAdd", False}, {"AddDeviceRescanTimer", 0}});
    setUpManager(Dict<IString, IBaseObject>({{"ModuleManager", options}}));
    addDiscoveredDevice({{"IPv4"}});

    add("legdev://host", nullptr);
    ASSERT_EQ(module->scanCount, 0);

    auto flat = PropertyObject();
    flat.addProperty(BoolProperty("ScanOnAdd", true));
    add("legdev://host", flat);
    ASSERT_EQ(module->scanCount, 1);

    add("daq://m_s", flat);
    ASSERT_EQ(module->scanCount, 2);
}

TEST_F(LegacyAddDeviceTest, InvalidFlatValueFails)
{
    auto config = PropertyObject();
    config.addProperty(StringProperty("Port", "not a number"));

    ASSERT_THROW(add("legdev://host", config), InvalidValueException);
    ASSERT_TRUE(module->configs.empty());
}

TEST_F(LegacyAddDeviceTest, DeviceSectionWithoutTypeUsesGeneral)
{
    const auto config = legacyConfig();
    section(config, "Device").removeProperty("legdev");
    const auto general = section(config, "General");
    general.setPropertyValue("Username", "u");
    general.setPropertyValue("Password", "p");

    add("legdev://host", config);

    ASSERT_EQ(lastConfig(), with(defaults(), {{"Username", "u"}, {"Password", "p"}}));
}

TEST_F(LegacyAddDeviceTest, ConnectionStringUnknownOptionIgnored)
{
    add("legdev://host?Unknown=1", legacyConfig());

    ASSERT_EQ(lastConnectionString(), "legdev://host");
    ASSERT_EQ(lastConfig(), defaults());
}

TEST_F(LegacyAddDeviceTest, SmartPrimaryAddressType)
{
    addDiscoveredDevice({{"IPv4"}, {"IPv6"}});

    const auto config = legacyConfig();
    section(config, "General").setPropertyValue("PrimaryAddressType", "IPv6");
    add("daq://m_s", config);
    ASSERT_EQ(lastConnectionString(), "legdev://IPv6");
    ASSERT_EQ(lastConfig(), defaults());

    auto flat = PropertyObject();
    flat.addProperty(StringProperty("PrimaryAddressType", "IPv6"));
    add("daq://m_s", flat);
    ASSERT_EQ(lastConnectionString(), "legdev://IPv6");
    ASSERT_EQ(lastConfig(), defaults());
}

TEST_F(LegacyAddDeviceTest, SmartPrimaryAddressTypeMissing)
{
    addDiscoveredDevice({{"IPv4"}});

    const auto config = legacyConfig();
    section(config, "General").setPropertyValue("PrimaryAddressType", "IPv6");

    add("daq://m_s", config);
    ASSERT_EQ(lastConnectionString(), "legdev://IPv4");
}

TEST_F(LegacyAddDeviceTest, SmartBlankAddressTypePrefersIPv4)
{
    addDiscoveredDevice({{"IPv6"}, {"IPv4"}});

    add("daq://m_s", nullptr);
    ASSERT_EQ(lastConnectionString(), "legdev://IPv4");

    add("daq://m_s", legacyConfig());
    ASSERT_EQ(lastConnectionString(), "legdev://IPv4");
}

TEST_F(LegacyAddDeviceTest, SmartPrimaryAddressTypeTakesFirstOfType)
{
    addDiscoveredDevice({{"IPv4"}, {"IPv6", "first"}, {"IPv6", "second"}});

    const auto config = legacyConfig();
    section(config, "General").setPropertyValue("PrimaryAddressType", "IPv6");

    add("daq://m_s", config);
    ASSERT_EQ(lastConnectionString(), "legdev://first");
}

TEST_F(LegacyAddDeviceTest, SmartDeviceSectionAndGeneral)
{
    addDiscoveredDevice({{"IPv4"}});

    const auto config = legacyConfig();
    section(config, "Device.legdev").setPropertyValue("Port", 1);
    section(config, "General").setPropertyValue("Username", "u");

    add("daq://m_s", config);
    ASSERT_EQ(lastConfig(), with(defaults(), {{"Port", "1"}, {"Username", "u"}}));
}
