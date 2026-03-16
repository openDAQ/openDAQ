#include "test_helpers/device_modules.h"
#include "test_helpers/test_helpers.h"
#include <opendaq/update_parameters_factory.h>

using namespace daq;

// TODO: Refactor test suite to reduce code duplication.

class RemoteModulesUpdateTest : public testing::Test
{
protected:
    RemoteModulesUpdateTest()
    {
    }

    static void configureServerInstance(InstancePtr& instance, int nativePort, int opcuaPort, const StringPtr& serialNumber, bool configureFB)
    {
        instance = InstanceBuilder()
            .setModulePath("[[none]]")
            .setGlobalLogLevel(LogLevel::Warn)
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
        if (configureFB)
        {
            auto rootFb = rootDevice.addFunctionBlock("RefFBModuleScaling");
            auto childFb = child.addFunctionBlock("RefFBModuleScaling");

            rootFb.getInputPorts()[0].connect(child.getSignalsRecursive()[0]);
            childFb.getInputPorts()[0].connect(rootDevice.getSignalsRecursive()[0]);
        }

        
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
            .setGlobalLogLevel(LogLevel::Warn)
            .build();

        addNativeClientModule(instance);

        instance.addDevice("daq.nd://127.0.0.1:7420");
        instance.addDevice("daq.nd://127.0.0.1:7421");
    }

    static void configureOpcuaClientInstance(InstancePtr& instance)
    {        
        instance = InstanceBuilder()
            .setModulePath("[[none]]")
            .setGlobalLogLevel(LogLevel::Warn)
            .build();

        addOpcuaClientModule(instance);

        instance.addDevice("daq.opcua://127.0.0.1:4840");
        instance.addDevice("daq.opcua://127.0.0.1:4841");
    }

    static void clearConnections(const InstancePtr& instance)
    {
        instance.getFunctionBlocks()[0].getInputPorts()[0].disconnect();
        instance.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].disconnect();
    }

    void SetUp() override
    {
        configureServerInstance(serverInstance1, 7420, 4840, "Test1", true);
        configureServerInstance(serverInstance2, 7421, 4841, "Test2", true);
        configureServerInstance(serverInstance3, 7422, 4842, "Test3", false);
    }

    InstancePtr serverInstance1;
    InstancePtr serverInstance2;
    InstancePtr serverInstance3;
};

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnectionsNative)
{     
    UpdateParametersPtr params;
    StringPtr serializeStr;
    {
        InstancePtr instance;
        configureNativeClientInstance(instance);

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
        test2Options.setNewSerialNumber("Test1");

        params = UpdateParameters();
        params.setDeviceUpdateOptions(options);
        instance.loadConfiguration(serializeStr, params);
        
        ASSERT_EQ(instance.getDevices()[0].getLocalId(), "openDAQ_Test3");
        ASSERT_EQ(instance.getDevices()[1].getLocalId(), "openDAQ_Test1");

        auto root1 = instance.getDevices()[0];
        auto child1 = root1.getDevices()[0];
        auto root2 = instance.getDevices()[1];
        auto child2 = root2.getDevices()[0];

        ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getSignalsRecursive()[0]);

        ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
        ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
        ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
        ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);
    }
    {
        auto freshInstance = InstanceBuilder()
        .setModulePath("[[none]]")
        .build();

        addNativeClientModule(freshInstance);

        clearConnections(serverInstance1);
        clearConnections(serverInstance2);
        clearConnections(serverInstance3);

        freshInstance.loadConfiguration(serializeStr, params);

        ASSERT_EQ(serverInstance1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance1.getDevices()[0].getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance1.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),serverInstance1.getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),serverInstance3.getSignalsRecursive()[0]);

        ASSERT_EQ(freshInstance.getDevices()[0].getLocalId(), "openDAQ_Test3");
        ASSERT_EQ(freshInstance.getDevices()[1].getLocalId(), "openDAQ_Test1");

        auto root1 = freshInstance.getDevices()[0];
        auto child1 = root1.getDevices()[0];
        auto root2 = freshInstance.getDevices()[1];
        auto child2 = root2.getDevices()[0];
        
        ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
        ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
        ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
        ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);
    }
}

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnectionsOpcUa)
{     
    UpdateParametersPtr params;
    StringPtr serializeStr;
    {
        InstancePtr instance;
        configureOpcuaClientInstance(instance);

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
        test2Options.setNewSerialNumber("Test1");

        params = UpdateParameters();
        params.setDeviceUpdateOptions(options);
        instance.loadConfiguration(serializeStr, params);
        
        ASSERT_EQ(instance.getDevices()[0].getLocalId(), "openDAQ_Test3");
        ASSERT_EQ(instance.getDevices()[1].getLocalId(), "openDAQ_Test1");

        auto root1 = instance.getDevices()[0];
        auto child1 = root1.getDevices()[0];
        auto root2 = instance.getDevices()[1];
        auto child2 = root2.getDevices()[0];

        ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getSignalsRecursive()[0]);

        ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
        ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
        ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
        ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);
    }
    {
        auto freshInstance = InstanceBuilder()
        .setModulePath("[[none]]")
        .build();

        addOpcuaClientModule(freshInstance);

        clearConnections(serverInstance1);
        clearConnections(serverInstance2);
        clearConnections(serverInstance3);

        freshInstance.loadConfiguration(serializeStr, params);

        ASSERT_EQ(serverInstance1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance1.getDevices()[0].getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance1.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),serverInstance1.getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
        ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),serverInstance3.getSignalsRecursive()[0]);

        ASSERT_EQ(freshInstance.getDevices()[0].getLocalId(), "openDAQ_Test3");
        ASSERT_EQ(freshInstance.getDevices()[1].getLocalId(), "openDAQ_Test1");

        auto root1 = freshInstance.getDevices()[0];
        auto child1 = root1.getDevices()[0];
        auto root2 = freshInstance.getDevices()[1];
        auto child2 = root2.getDevices()[0];
        
        ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
        ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
        ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
        ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);
    }
}

TEST_F(RemoteModulesUpdateTest, GatewayDeviceRemapNative)
{
    
}

TEST_F(RemoteModulesUpdateTest, RemapWithConnectionStringNative)
{
    
}

TEST_F(RemoteModulesUpdateTest, RemapWithConnectionStringOPCUA)
{
    
}

TEST_F(RemoteModulesUpdateTest, LoadOnlyNative)
{
    
}

TEST_F(RemoteModulesUpdateTest, LoadOnlyOPCUA)
{
    
}

TEST_F(RemoteModulesUpdateTest, SkipNative)
{
    
}

TEST_F(RemoteModulesUpdateTest, SkipOPCUA)
{
    
}
