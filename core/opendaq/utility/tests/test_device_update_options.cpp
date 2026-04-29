#include <gtest/gtest.h>
#include <opendaq/module_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/function_block_impl.h>
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
        this->objPtr.addProperty(StringProperty("Test", "Unchanged"));
        createAndAddSignal("TestSig");

        auto fb = createWithImplementation<IFunctionBlock, FunctionBlock>(FunctionBlockType("test", "", ""), ctx, this->functionBlocks, "TestFB");
        FolderConfigPtr ips = fb.getItem("IP");
        auto ip = InputPort(ctx, ips, "TestIP");

        ips.addItem(ip);
        this->functionBlocks.addItem(fb);
    }

private:
    DeviceInfoPtr onGetInfo() override
    {
        auto info = DeviceInfo(fmt::format("daqtest://Man{}_Ser{}", id, id));
        info.setManufacturer(fmt::format("Man{}", id));
        info.setSerialNumber(fmt::format("Ser{}", id));
        info.setDeviceType(DeviceType("Test", "Test", "", "daqtest"));
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
        for (int i = 0; i < 5; ++i)
        {
            auto info = DeviceInfo(fmt::format("daqtest://Man{}_Ser{}", i, i));
            info.setManufacturer(fmt::format("Man{}", i));
            info.setSerialNumber(fmt::format("Ser{}", i));
            
            auto cap = ServerCapability("test", "test", ProtocolType::Unknown);
            cap.setConnectionString(fmt::format("daqtest://Man{}_Ser{}", i, i));
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
        
        // Root <Man0_Ser0>
        //   - child1 <Man1_Ser1>
        //   - child2 <Man2_Ser2>
        //       - child2_1 <Man3_Ser3>
        //       - child2_2 <Man4_Ser4>

        instance = InstanceCustom(context, "localInstance");
        instance.setRootDevice("daqtest://Man0_Ser0");
        auto child1 = instance.addDevice("daqtest://Man1_Ser1");
        auto child2 = instance.addDevice("daqtest://Man2_Ser2");
        auto child2_1 = child2.addDevice("daqtest://Man3_Ser3");
        auto child2_2 = child2.addDevice("daqtest://Man4_Ser4");

        // child1.sig -> child2_1.ip
        // child2_2.sig -> child2.ip
        child2_1.getFunctionBlocks()[0].getInputPorts()[0].connect(child1.getSignals()[0]);
        child2.getFunctionBlocks()[0].getInputPorts()[0].connect(child2_2.getSignals()[0]);
        
        const auto logger1 = Logger();
        const auto moduleManager1 = ModuleManager("[[none]]");
        const auto authenticationProvider1 = AuthenticationProvider();
        const auto context1 = Context(Scheduler(logger1), logger1, TypeManager(), moduleManager1, authenticationProvider1);
        moduleManager1.addModule(createWithImplementation<IModule, UpdateOptionsTestDeviceModule>(context1));

        freshInstance = InstanceCustom(context1, "localInstance");
        freshInstance.setRootDevice("daqtest://Man0_Ser0");
    }

    InstancePtr instance;
    InstancePtr freshInstance;
};

TEST_F(DeviceUpdateOptionsTest, Parse)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();

    auto options = DeviceUpdateOptions(str);
    
    ASSERT_EQ(options.getLocalId(), "Man0_Ser0");
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
    ASSERT_EQ(child1Options.getLocalId(), "Man1_Ser1");
    ASSERT_EQ(child2Options.getLocalId(), "Man2_Ser2");
    ASSERT_EQ(child2_1Options.getLocalId(), "Man3_Ser3");
    ASSERT_EQ(child2_2Options.getLocalId(), "Man4_Ser4");

    // Manufacturer
    ASSERT_EQ(child1Options.getManufacturer(), "Man1");
    ASSERT_EQ(child2Options.getManufacturer(), "Man2");
    ASSERT_EQ(child2_1Options.getManufacturer(), "Man3");
    ASSERT_EQ(child2_2Options.getManufacturer(), "Man4");

    // Serial Number
    ASSERT_EQ(child1Options.getSerialNumber(), "Ser1");
    ASSERT_EQ(child2Options.getSerialNumber(), "Ser2");
    ASSERT_EQ(child2_1Options.getSerialNumber(), "Ser3");
    ASSERT_EQ(child2_2Options.getSerialNumber(), "Ser4");

    // Connection String
    ASSERT_EQ(child1Options.getConnectionString(), "daqtest://Man1_Ser1");
    ASSERT_EQ(child2Options.getConnectionString(), "daqtest://Man2_Ser2");
    ASSERT_EQ(child2_1Options.getConnectionString(), "daqtest://Man3_Ser3");
    ASSERT_EQ(child2_2Options.getConnectionString(), "daqtest://Man4_Ser4");
}

TEST_F(DeviceUpdateOptionsTest, RemapFreshInstance)
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
    child1Options.setNewManufacturer("Man4");
    child1Options.setNewSerialNumber("Ser4");
    
    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewManufacturer("Man1");
    child2ChildOptions[1].setNewSerialNumber("Ser1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

    ASSERT_EQ(freshInstance.getDevices()[0].getLocalId(), "Man4_Ser4");
    ASSERT_EQ(freshInstance.getDevices()[0].getInfo().getManufacturer(), "Man4");
    ASSERT_EQ(freshInstance.getDevices()[0].getInfo().getSerialNumber(), "Ser4");

    ASSERT_EQ(freshInstance.getDevices()[1].getDevices()[1].getLocalId(), "Man1_Ser1");
    ASSERT_EQ(freshInstance.getDevices()[1].getDevices()[1].getInfo().getManufacturer(), "Man1");
    ASSERT_EQ(freshInstance.getDevices()[1].getDevices()[1].getInfo().getSerialNumber(), "Ser1");
}

TEST_F(DeviceUpdateOptionsTest, RemapOldInstance)
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
    child1Options.setNewManufacturer("Man4");
    child1Options.setNewSerialNumber("Ser4");
    
    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewManufacturer("Man1");
    child2ChildOptions[1].setNewSerialNumber("Ser1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    // Order is swapped due to re-add on remapping
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man4_Ser4");
    ASSERT_EQ(instance.getDevices()[1].getInfo().getManufacturer(), "Man4");
    ASSERT_EQ(instance.getDevices()[1].getInfo().getSerialNumber(), "Ser4");
              
    // Order of leaf devices stays the same, as the device was already the last in the list.
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getLocalId(), "Man1_Ser1");
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getInfo().getManufacturer(), "Man1");
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getInfo().getSerialNumber(), "Ser1");
}

TEST_F(DeviceUpdateOptionsTest, RemapWithConnectionString)
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
    child1Options.setNewConnectionString("daqtest://Man4_Ser4");

    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewConnectionString("daqtest://Man1_Ser1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    // Order is swapped due to re-add on remapping
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man4_Ser4");
    ASSERT_EQ(instance.getDevices()[1].getInfo().getManufacturer(), "Man4");
    ASSERT_EQ(instance.getDevices()[1].getInfo().getSerialNumber(), "Ser4");
              
    // Order of leaf devices stays the same, as the device was already the last in the list.
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getLocalId(), "Man1_Ser1");
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getInfo().getManufacturer(), "Man1");
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getInfo().getSerialNumber(), "Ser1");
}

TEST_F(DeviceUpdateOptionsTest, RemapSettingsPriority)
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
    child1Options.setNewManufacturer("Man4");
    child1Options.setNewSerialNumber("Ser4");
    child1Options.setNewConnectionString("daqtest://invalid");
    
    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewManufacturer("Man1");
    child2ChildOptions[1].setNewSerialNumber("Ser1");
    child2ChildOptions[1].setNewConnectionString("daqtest://invalid");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    // Order is swapped due to re-add on remapping
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man4_Ser4");
    ASSERT_EQ(instance.getDevices()[1].getInfo().getManufacturer(), "Man4");
    ASSERT_EQ(instance.getDevices()[1].getInfo().getSerialNumber(), "Ser4");
              
    // Order of leaf devices stays the same, as the device was already the last in the list.
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getLocalId(), "Man1_Ser1");
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getInfo().getManufacturer(), "Man1");
    ASSERT_EQ(instance.getDevices()[0].getDevices()[1].getInfo().getSerialNumber(), "Ser1");
}

TEST_F(DeviceUpdateOptionsTest, RemapPropChanges)
{
    instance.getDevices()[0].setPropertyValue("Test", "Changed");
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    
    instance.getDevices()[0].setPropertyValue("Test", "Unchanged");

    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto child1Options = rootChildOptions[0];
    auto child2Options = rootChildOptions[1];
    auto child2ChildOptions = child2Options.getChildDeviceOptions();

    child1Options.setUpdateMode(DeviceUpdateMode::Remap);
    child1Options.setNewManufacturer("Man4");
    child1Options.setNewSerialNumber("Ser4");
    
    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewManufacturer("Man1");
    child2ChildOptions[1].setNewSerialNumber("Ser1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man4_Ser4");
    ASSERT_EQ(instance.getDevices()[1].getPropertyValue("Test"), "Changed");
}

TEST_F(DeviceUpdateOptionsTest, RemapFailed)
{
    instance.getDevices()[1].setPropertyValue("Test", "Changed");
    instance.getDevices()[1].getDevices()[0].setPropertyValue("Test", "Changed");
    instance.getDevices()[1].getDevices()[1].setPropertyValue("Test", "Changed");

    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    rootChildOptions[0].setUpdateMode(DeviceUpdateMode::Remap);
    rootChildOptions[0].setNewManufacturer("Invalid");
    rootChildOptions[0].setNewSerialNumber("Invalid");
    rootChildOptions[0].setNewConnectionString("");
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices().getCount(), 1u);
}

TEST_F(DeviceUpdateOptionsTest, Skip)
{
    instance.getDevices()[1].setPropertyValue("Test", "Changed");
    instance.getDevices()[1].getDevices()[0].setPropertyValue("Test", "Changed");
    instance.getDevices()[1].getDevices()[1].setPropertyValue("Test", "Changed");

    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();

    instance.getDevices()[1].setPropertyValue("Test", "Unchanged");
    instance.getDevices()[1].getDevices()[0].setPropertyValue("Test", "Unchanged");
    instance.getDevices()[1].getDevices()[1].setPropertyValue("Test", "Unchanged");

    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    rootChildOptions[1].setUpdateMode(DeviceUpdateMode::Skip);
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices()[1].getPropertyValue("Test"), "Unchanged");
    ASSERT_EQ(instance.getDevices()[1].getDevices()[0].getPropertyValue("Test"), "Unchanged");
    ASSERT_EQ(instance.getDevices()[1].getDevices()[1].getPropertyValue("Test"), "Unchanged");
}

TEST_F(DeviceUpdateOptionsTest, Remove)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    rootChildOptions[0].setUpdateMode(DeviceUpdateMode::Remove);
    rootChildOptions[1].getChildDeviceOptions()[0].setUpdateMode(DeviceUpdateMode::Remove);
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);
    
    ASSERT_EQ(instance.getDevices().getCount(), 1u);
    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "Man2_Ser2");
    ASSERT_EQ(instance.getDevices()[0].getDevices().getCount(), 1u);
    ASSERT_EQ(instance.getDevices()[0].getDevices()[0].getLocalId(), "Man4_Ser4");
}

TEST_F(DeviceUpdateOptionsTest, UpdateOnly)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    
    instance.removeDevice(instance.getDevices()[0]);
    auto child = instance.getDevices()[0];
    child.removeDevice(child.getDevices()[1]);

    ASSERT_EQ(instance.getDevices().getCount(), 1u);
    ASSERT_EQ(child.getDevices().getCount(), 1u);

    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    rootChildOptions[0].setUpdateMode(DeviceUpdateMode::UpdateOnly);
    rootChildOptions[1].getChildDeviceOptions()[1].setUpdateMode(DeviceUpdateMode::UpdateOnly);
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices().getCount(), 1u);
    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "Man2_Ser2");
    ASSERT_EQ(instance.getDevices()[0].getDevices().getCount(), 1u);
    ASSERT_EQ(instance.getDevices()[0].getDevices()[0].getLocalId(), "Man3_Ser3");
}

TEST_F(DeviceUpdateOptionsTest, CheckDefaultSettings)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();

    auto options = DeviceUpdateOptions(str);
    ASSERT_EQ(options.getNewManufacturer(), "");
    ASSERT_EQ(options.getNewSerialNumber(), "");
    ASSERT_EQ(options.getNewConnectionString(), "");
    ASSERT_EQ(options.getUpdateMode(), DeviceUpdateMode::Load);
}

TEST_F(DeviceUpdateOptionsTest, RemapCheckIPConnections)
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
    child1Options.setNewManufacturer("Man4");
    child1Options.setNewSerialNumber("Ser4");

    child2Options.setUpdateMode(DeviceUpdateMode::Remap);
    child2Options.setNewManufacturer("Man3");
    child2Options.setNewSerialNumber("Ser3");
    
    child2ChildOptions[0].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[0].setNewManufacturer("Man2");
    child2ChildOptions[0].setNewSerialNumber("Ser2");

    child2ChildOptions[1].setUpdateMode(DeviceUpdateMode::Remap);
    child2ChildOptions[1].setNewManufacturer("Man1");
    child2ChildOptions[1].setNewSerialNumber("Ser1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

    auto child1 = freshInstance.getDevices()[0];
    auto child2 = freshInstance.getDevices()[1];
    auto child2_1 = child2.getDevices()[0];
    auto child2_2 = child2.getDevices()[1];

    // Swaps:
    // child1 <Man1_Ser1> <-> child2_2 <Man4_Ser4>
    // child2 <Man2_Ser2> <-> child2_1 <Man3_Ser3>
    ASSERT_EQ(child1.getLocalId(), "Man4_Ser4");
    ASSERT_EQ(child2.getLocalId(), "Man3_Ser3");
    ASSERT_EQ(child2_1.getLocalId(), "Man2_Ser2");
    ASSERT_EQ(child2_2.getLocalId(), "Man1_Ser1");
    
    // Connections are preserved:
    // child1.sig -> child2_1.ip
    // child2_2.sig -> child2.ip
    ASSERT_EQ(child2_1.getFunctionBlocks()[0].getInputPorts()[0].getConnection().getSignal(), child1.getSignals()[0]);
    ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getConnection().getSignal(), child2_2.getSignals()[0]);
}

TEST_F(DeviceUpdateOptionsTest, SerializeDeserialize)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);

    auto serializer2 = JsonSerializer();
    options.serialize(serializer2);

    auto deserializer = JsonDeserializer();
    DeviceUpdateOptionsPtr optionsDeserialized = deserializer.deserialize(serializer2.getOutput());

    ASSERT_EQ(options, optionsDeserialized);
}

TEST_F(DeviceUpdateOptionsTest, MergeKeepsExistingDeviceNotInConfiguration)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);

    // Add an extra device after the configuration was saved.
    auto extraDevice = instance.addDevice("daqtest://Man5_Ser5");
    ASSERT_EQ(extraDevice.getLocalId(), "Man5_Ser5");
    ASSERT_EQ(instance.getDevices().getCount(), 3u);

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    params.setRemoveOldDevices(False);

    instance.loadConfiguration(str, params);

    // Merge keeps devices that are not mentioned in the loaded configuration.
    ASSERT_EQ(instance.getDevices().getCount(), 3u);
    ASSERT_EQ(instance.getDevices()[2].getLocalId(), "Man5_Ser5");

    // Original saved structure is still present as well.
    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "Man1_Ser1");
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man2_Ser2");
    ASSERT_EQ(instance.getDevices()[1].getDevices().getCount(), 2u);
}

TEST_F(DeviceUpdateOptionsTest, ExactRemovesExistingDeviceNotInConfiguration)
{
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);

    // Add an extra device after the configuration was saved.
    auto extraDevice = instance.addDevice("daqtest://Man5_Ser5");
    ASSERT_EQ(extraDevice.getLocalId(), "Man5_Ser5");
    ASSERT_EQ(instance.getDevices().getCount(), 3u);

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    params.setRemoveOldDevices(True);

    instance.loadConfiguration(str, params);

    // Exact removes devices that are not mentioned in the loaded configuration.
    ASSERT_EQ(instance.getDevices().getCount(), 2u);

    bool foundExtraDevice = false;
    for (SizeT i = 0; i < instance.getDevices().getCount(); ++i)
    {
        if (instance.getDevices()[i].getLocalId() == "Man5_Ser5")
        {
            foundExtraDevice = true;
            break;
        }
    }
    ASSERT_FALSE(foundExtraDevice);

    // The resulting tree matches the saved configuration exactly.
    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "Man1_Ser1");
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man2_Ser2");
    ASSERT_EQ(instance.getDevices()[1].getDevices().getCount(), 2u);
    ASSERT_EQ(instance.getDevices()[1].getDevices()[0].getLocalId(), "Man3_Ser3");
    ASSERT_EQ(instance.getDevices()[1].getDevices()[1].getLocalId(), "Man4_Ser4");
}

TEST_F(DeviceUpdateOptionsTest, RemoveOldInteractionWithRemap)
{
    auto childDevice2 = instance.getDevices()[1];
    childDevice2.removeDevice(childDevice2.getDevices()[1]); // Remove Man4_Ser4
    ASSERT_EQ(childDevice2.getDevices().getCount(), 1u);
    ASSERT_EQ(childDevice2.getDevices()[0].getLocalId(), "Man3_Ser3");

    // Man0
    // - Man1
    // - Man2
    //   - Man3
    // This config is saved
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);

    // Add an extra device after the configuration was saved.
    auto extraDevice = childDevice2.addDevice("daqtest://Man4_Ser4");
    ASSERT_EQ(extraDevice.getLocalId(), "Man4_Ser4");
    ASSERT_EQ(instance.getDevices().getCount(), 2u);
    ASSERT_EQ(childDevice2.getDevices().getCount(), 2u);
    // Man0
    // - Man1
    // - Man2
    //   - Man3
    //   - Man4
    // This is the "dirty state we start from"

    // References to device options
    auto rootChildOptions = options.getChildDeviceOptions();
    auto child1Options = rootChildOptions[0];
    child1Options.setUpdateMode(DeviceUpdateMode::Remap);
    child1Options.setNewManufacturer("Man4");
    child1Options.setNewSerialNumber("Ser4");
    // We request remap Man1 -> Man4

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    params.setRemoveOldDevices(True); // Remove devices not in config
    // The expected result is
    // Man0
    // - Man2
    //   - Man3
    //  (x Man4 removed due to not being present in the loaded config)
    // - Man4 (Remapped from Man1)
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices().getCount(), 2u);
    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "Man2_Ser2");
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "Man4_Ser4");

    // Removed the Man4 since it was not in the original config
    ASSERT_EQ(instance.getDevices()[0].getDevices().getCount(), 1u);
    ASSERT_EQ(instance.getDevices()[0].getDevices()[0].getLocalId(), "Man3_Ser3");
}
