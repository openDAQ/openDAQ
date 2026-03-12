#include "test_helpers/device_modules.h"
#include "test_helpers/test_helpers.h"
#include <opendaq/update_parameters_factory.h>

using namespace daq;

class RemoteModulesUpdateTest : public testing::Test
{
protected:
    RemoteModulesUpdateTest()
    {
    }

    static void configureServerInstance(InstancePtr& instance, int nativePort, int opcuaPort, const StringPtr& serialNumber)
    {
        instance = InstanceBuilder()
            .setModulePath("[[none]]")
            .addDiscoveryServer("mdns")
            .build();

        addNativeServerModule(instance);
        addOpcuaServerModule(instance);
        addRefDeviceModule(instance);
        addRefFBModule(instance);
        
        auto deviceConfig = instance.getAvailableDeviceTypes().get("daqref").createDefaultConfig();
        deviceConfig.setPropertyValue("SerialNumber", serialNumber);
        deviceConfig.setPropertyValue("LocalId", "openDAQ_" + serialNumber);
        instance.setRootDevice("daqref://device0", deviceConfig);
        auto rootDevice = instance.getRootDevice();
        auto child = rootDevice.addDevice("daqref://device1");
        auto rootFb = rootDevice.addFunctionBlock("RefFBModuleScaling");
        auto childFb = child.addFunctionBlock("RefFBModuleScaling");

        rootFb.getInputPorts()[0].connect(child.getSignalsRecursive()[0]);
        childFb.getInputPorts()[0].connect(rootDevice.getSignalsRecursive()[0]);
        
        auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
        serverConfig.setPropertyValue("NativeStreamingPort", nativePort);
        instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

        serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
        serverConfig.setPropertyValue("Port", opcuaPort);
        instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();
    }

    static void configureNativeClientInstance(InstancePtr& instance)
    {        
        instance = InstanceBuilder()
            .setModulePath("[[none]]")
            .build();

        addNativeClientModule(instance);

        auto deviceConfig = instance.getAvailableDeviceTypes().get("OpenDAQNativeConfiguration").createDefaultConfig();
        PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
        transportLayerConfig.setPropertyValue("MonitoringEnabled", false);
        transportLayerConfig.setPropertyValue("HeartbeatPeriod", 100000);
        transportLayerConfig.setPropertyValue("InactivityTimeout", 150000);
        transportLayerConfig.setPropertyValue("ConnectionTimeout", 100000);

        instance.addDevice("daq.nd://127.0.0.1:7420", deviceConfig);
        instance.addDevice("daq.nd://127.0.0.1:7421", deviceConfig);
    }

    void SetUp() override
    {
        configureServerInstance(serverInstance1, 7420, 4840, "Test1");
        configureServerInstance(serverInstance2, 7421, 4841, "Test2");
        configureServerInstance(serverInstance3, 7422, 4842, "Test3");

    }

    InstancePtr serverInstance1;
    InstancePtr serverInstance2;
    InstancePtr serverInstance3;
};

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnections)
{     
    UpdateParametersPtr params;
    StringPtr serializeStr;
    {
        
        InstancePtr instance;
        configureNativeClientInstance(instance);
        
        auto root1 = instance.getDevices()[0];
        auto child1 = root1.getDevices()[0];
        auto root2 = instance.getDevices()[0];
        auto child2 = root2.getDevices()[0];
        //ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getConnection().getSignal(), child1.getSignalsRecursive()[0]);
        //ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getConnection().getSignal(), root1.getSignalsRecursive()[0]);

        auto serializer = JsonSerializer();
        instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
        serializeStr = serializer.getOutput();

        auto options = DeviceUpdateOptions(serializeStr);
        auto rootChildOptions = options.getChildDeviceOptions();
        auto test1Options = rootChildOptions[0];
        auto test2Options = rootChildOptions[1];
        
        test1Options.setUpdateMode(DeviceUpdateMode::Remap);
        test1Options.setNewManufacturer("openDAQ");
        test1Options.setNewSerialNumber("Test3");

        test2Options.setUpdateMode(DeviceUpdateMode::Remap);
        test2Options.setNewManufacturer("openDAQ");
        test2Options.setNewSerialNumber("Test2");

        params = UpdateParameters();
        params.setDeviceUpdateOptions(options);
        instance.loadConfiguration(serializeStr, params);
        
        ASSERT_EQ(instance.getDevices()[0].getLocalId(), "openDAQ_Test3");
        ASSERT_EQ(instance.getDevices()[1].getLocalId(), "openDAQ_Test2");

        root1 = instance.getDevices()[0];
        child1 = root1.getDevices()[0];
        root2 = instance.getDevices()[0];
        child2 = root2.getDevices()[0];

        //ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getConnection().getSignal(), child1.getSignalsRecursive()[0]);
        //ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getConnection().getSignal(), root1.getSignalsRecursive()[0]);
    }


    auto freshInstance = InstanceBuilder()
        .setModulePath("[[none]]")
        .build();

    addNativeClientModule(freshInstance);

    freshInstance.loadConfiguration(serializeStr, params);

    ASSERT_EQ(freshInstance.getDevices()[0].getLocalId(), "openDAQ_Test3");
    ASSERT_EQ(freshInstance.getDevices()[1].getLocalId(), "openDAQ_Test2");
}
