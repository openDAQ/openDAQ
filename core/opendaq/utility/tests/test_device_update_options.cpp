#include <gtest/gtest.h>
#include <opendaq/module_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_update_options_factory.h>
#include <coreobjects/authentication_provider_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/instance_factory.h>

using namespace daq;

class UpdateOptionsTestDevice : public Device
{
public:
    UpdateOptionsTestDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
        this->id = localId.toStdString().back() - '0';
    }

private:
    DeviceInfoPtr onGetInfo() override
    {
        auto info = DeviceInfo(fmt::format("daqtest://Test{}_Test{}", id, id));
        info.setManufacturer(fmt::format("Test{}", id));
        info.setSerialNumber(fmt::format("Test{}", id));
        return info;
    }

    bool allowAddDevicesFromModules() override
    {
        return true;
    }

    int id;
};

class UpdateOptionsTestDeviceModule : public Module
{
public:
    UpdateOptionsTestDeviceModule(ContextPtr context)
        : Module("UpdateOptionsTest", VersionInfo(1, 0, 0), std::move(context), "UpdateOptionsTest")
    {
    }
    
    ListPtr<IDeviceInfo> onGetAvailableDevices() override
    {
        auto result = List<IDeviceInfo>();
        for (int i = 0; i < 10; ++i)
        {
            auto info = DeviceInfo(fmt::format("daqtest://Test{}_Test{}", i, i));
            info.setManufacturer(fmt::format("Test{}", i));
            info.setSerialNumber(fmt::format("Test{}", i));
            
            auto cap = ServerCapability("test", "test", ProtocolType::Unknown);
            cap.setConnectionString(fmt::format("daqtest://Test{}_Test{}", i, i));
            info.asPtr<IDeviceInfoInternal>().addServerCapability(cap);

            result.pushBack(info);
        }

        return result;
    }

    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override
    {
        auto result = Dict<IString, IDeviceType>();

        auto deviceType = DeviceType("Test", "Test", "", "daqtest");
        result.set(deviceType.getId(), deviceType);

        return result;
    }

    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr&) override
    {
        return createWithImplementation<IDevice, UpdateOptionsTestDevice>(this->context, parent, getIdFromConnectionString(connectionString));
    }

    static std::string getIdFromConnectionString(const std::string& connectionString)
    {
        std::string prefixWithDeviceStr = "daqtest://";
        return connectionString.substr(prefixWithDeviceStr.size(), std::string::npos);
    }
};

class DeviceUpdateOptionsTest : public testing::Test
{
protected:
    DeviceUpdateOptionsTest()
    {
    }

    void SetUp() override
    {
        const auto logger = Logger();
        const auto moduleManager = ModuleManager("[[none]]");
        const auto authenticationProvider = AuthenticationProvider();
        const auto context = Context(Scheduler(logger), logger, TypeManager(), moduleManager, authenticationProvider);
        moduleManager.addModule(createWithImplementation<IModule, UpdateOptionsTestDeviceModule>(context));
        
        instance = InstanceCustom(context, "localInstance");
        instance.setRootDevice("daqtest://Test0_Test0");
        instance.addDevice("daqtest://Test1_Test1");
        auto child2 = instance.addDevice("daqtest://Test2_Test2");
        child2.addDevice("daqtest://Test3_Test3");
        child2.addDevice("daqtest://Test4_Test4");

        
        freshInstance = InstanceCustom(context, "localInstance");
    }

    InstancePtr instance;
    InstancePtr freshInstance;
};

TEST_F(DeviceUpdateOptionsTest, DeviceUpdateOptionsParse)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();

    auto options = DeviceUpdateOptions(str);
    
    ASSERT_EQ(options.getLocalId(), "Test0_Test0");
    auto rootChildOptions = options.getChildDeviceOptions();
    ASSERT_EQ(rootChildOptions.getCount(), 2u);

    auto child1Options = rootChildOptions[0];
    ASSERT_EQ(child1Options.getChildDeviceOptions().getCount(), 0u);

    auto child2Options = rootChildOptions[1];
    auto child2ChildOptions = child2Options.getChildDeviceOptions();
    ASSERT_EQ(child2ChildOptions.getCount(), 2u);

    auto child2_1Options = child2ChildOptions[0];
    auto child2_2Options = child2ChildOptions[1];
    ASSERT_EQ(child2_1Options.getChildDeviceOptions().getCount(), 0u);
    ASSERT_EQ(child2_2Options.getChildDeviceOptions().getCount(), 0u);

    // Local IDs
    ASSERT_EQ(child1Options.getLocalId(), "Test1_Test1");
    ASSERT_EQ(child2Options.getLocalId(), "Test2_Test2");
    ASSERT_EQ(child2_1Options.getLocalId(), "Test3_Test3");
    ASSERT_EQ(child2_2Options.getLocalId(), "Test4_Test4");

    // Manufacturer
    ASSERT_EQ(child1Options.getManufacturer(), "Test1");
    ASSERT_EQ(child2Options.getManufacturer(), "Test2");
    ASSERT_EQ(child2_1Options.getManufacturer(), "Test3");
    ASSERT_EQ(child2_2Options.getManufacturer(), "Test4");

    // Serial Number
    ASSERT_EQ(child1Options.getSerialNumber(), "Test1");
    ASSERT_EQ(child2Options.getSerialNumber(), "Test2");
    ASSERT_EQ(child2_1Options.getSerialNumber(), "Test3");
    ASSERT_EQ(child2_2Options.getSerialNumber(), "Test4");

    // Connection String
    ASSERT_EQ(child1Options.getConnectionString(), "daqtest://Test1_Test1");
    ASSERT_EQ(child2Options.getConnectionString(), "daqtest://Test2_Test2");
    ASSERT_EQ(child2_1Options.getConnectionString(), "daqtest://Test3_Test3");
    ASSERT_EQ(child2_2Options.getConnectionString(), "daqtest://Test4_Test4");
}

TEST_F(DeviceUpdateOptionsTest, DeviceUpdateOptionsRemap)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();

    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto child1Options = rootChildOptions[0];
    auto child2Options = rootChildOptions[1];
    auto child2ChildOptions = child2Options.getChildDeviceOptions();

    child1Options.setUpdateMode(DeviceUpdateMode::Remap);
    child1Options.setNewManufacturer("Test4");
    child1Options.setNewSerialNumber("Test4");
    
    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewManufacturer("Test1");
    child2ChildOptions[1].setNewSerialNumber("Test1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

    auto id1 = freshInstance.getDevices()[0].getLocalId();
    auto id2 = freshInstance.getDevices()[1].getLocalId();
    auto id3 = freshInstance.getDevices()[1].getDevices()[0].getLocalId();
    auto id4 = freshInstance.getDevices()[1].getDevices()[1].getLocalId();

    auto info1 = freshInstance.getDevices()[0].getInfo();
    auto info2 = freshInstance.getDevices()[1].getInfo();
    auto info3 = freshInstance.getDevices()[1].getDevices()[0].getInfo();
    auto info4 = freshInstance.getDevices()[1].getDevices()[1].getInfo();

    ASSERT_EQ(freshInstance.getDevices()[0].getInfo().getManufacturer(), "Test4");
    ASSERT_EQ(freshInstance.getDevices()[0].getInfo().getSerialNumber(), "Test4");

    ASSERT_EQ(freshInstance.getDevices()[1].getDevices()[1].getInfo().getSerialNumber(), "Test1");
    ASSERT_EQ(freshInstance.getDevices()[1].getDevices()[1].getInfo().getSerialNumber(), "Test1");
}

