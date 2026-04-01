#include "test_helpers/device_modules.h"
#include "test_helpers/test_helpers.h"
#include <opendaq/update_parameters_factory.h>

using namespace daq;

class RemoteModulesUpdateTest : public testing::Test
{
protected:

    static InstancePtr setupBaseInstance()
    {
        auto instance = InstanceBuilder()
            .setModulePath("[[none]]")
            .setGlobalLogLevel(LogLevel::Warn)
            .addDiscoveryServer("mdns")
            .build();

        addNativeServerModule(instance);
        addOpcuaServerModule(instance);
        addNativeClientModule(instance);
        addOpcuaClientModule(instance);
        addRefDeviceModule(instance);
        addRefFBModule(instance);

        return instance;
    }

    static void setupInstanceTree(const InstancePtr& instance, const StringPtr& serialNumber, bool addFunctionBlocks, bool addDevices = true)
    {
        auto deviceConfig = instance.getAvailableDeviceTypes().get("daqref").createDefaultConfig();
        deviceConfig.setPropertyValue("SerialNumber", serialNumber);
        deviceConfig.setPropertyValue("LocalId", "openDAQ_" + serialNumber);

        instance.setRootDevice("daqref://device0", deviceConfig);
        auto child = instance.addDevice("daqref://device1");

        if (addFunctionBlocks)
        {
            auto rootFb = instance.addFunctionBlock("RefFBModuleScaling");
            auto childFb = child.addFunctionBlock("RefFBModuleScaling");

            rootFb.getInputPorts()[0].connect(child.getSignalsRecursive()[0]);
            childFb.getInputPorts()[0].connect(instance.getSignalsRecursive()[0]);
        }
    }
     
    static void addServers(const InstancePtr& instance, int nativePort, int opcuaPort)
    {
        if (nativePort > 0)
        {
            auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
            serverConfig.setPropertyValue("NativeStreamingPort", nativePort);
            instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();
        }

        if (opcuaPort > 0)
        {
            auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
            serverConfig.setPropertyValue("Port", opcuaPort);
            instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();
        }
    }

    enum class ConnectionType
    {
        Native,
        Opcua
    };

    static void addDevice(const InstancePtr& instance, ConnectionType connectionType, int port)
    {        
        auto connectionString = connectionType == ConnectionType::Native
                                    ? "daq.nd://127.0.0.1:" + std::to_string(port)
                                    : "daq.opcua://127.0.0.1:" + std::to_string(port);
        PropertyObjectPtr deviceConfig;
        if (connectionType == ConnectionType::Opcua)
        {
            deviceConfig = PropertyObject();
        }
        else
        {
            deviceConfig = instance.getAvailableDeviceTypes().get("OpenDAQNativeConfiguration").createDefaultConfig();
            PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
            transportLayerConfig.setPropertyValue("MonitoringEnabled", false);
            transportLayerConfig.setPropertyValue("HeartbeatPeriod", 100000);
            transportLayerConfig.setPropertyValue("InactivityTimeout", 150000);
            transportLayerConfig.setPropertyValue("ConnectionTimeout", 100000);
        }

        instance.addDevice(connectionString, deviceConfig);
    }

    static void clearConnections(const InstancePtr& instance)
    {
        instance.getFunctionBlocks()[0].getInputPorts()[0].disconnect();
        instance.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].disconnect();
    }

    void configureServers()
    {
        serverInstance1 = setupBaseInstance();
        setupInstanceTree(serverInstance1, "Test1", true);
        addServers(serverInstance1, 7420, 4840);
        
        serverInstance2 = setupBaseInstance();
        setupInstanceTree(serverInstance2, "Test2", true);
        addServers(serverInstance2, 7421, 4841);
                
        serverInstance3 = setupBaseInstance();
        setupInstanceTree(serverInstance3, "Test3", false);
        addServers(serverInstance3, 7422, 4842);
    }

    void configureOpcuaClientInstance(const InstancePtr& instance)
    {
        addDevice(instance, ConnectionType::Opcua, 4840);
        addDevice(instance, ConnectionType::Opcua, 4841);
    }

    static void configureNativeClientInstance(const InstancePtr& instance)
    {
        addDevice(instance, ConnectionType::Native, 7420);
        addDevice(instance, ConnectionType::Native, 7421);
    }

    void SetUp() override
    {
        configureServers();
    }

    InstancePtr serverInstance1;
    InstancePtr serverInstance2;
    InstancePtr serverInstance3;

};

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnectionsNative)
{
    auto instance = setupBaseInstance();
    configureNativeClientInstance(instance);

    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewManufacturer("openDAQ");
    test1Options.setNewSerialNumber("Test3");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewManufacturer("openDAQ");
    test2Options.setNewSerialNumber("Test1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "openDAQ_Test3");
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "openDAQ_Test1");

    ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
    ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              serverInstance3.getSignalsRecursive()[0]);
        
    auto root1 = instance.getDevices()[0];
    auto child1 = root1.getDevices()[0];
    auto root2 = instance.getDevices()[1];
    auto child2 = root2.getDevices()[0];

    ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
    ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
    ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
    ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);
}

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnectionsNativeNewInstanceNative)
{
    auto instance = setupBaseInstance();
    configureNativeClientInstance(instance);
    
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewManufacturer("openDAQ");
    test1Options.setNewSerialNumber("Test3");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewManufacturer("openDAQ");
    test2Options.setNewSerialNumber("Test1");

    auto freshInstance = setupBaseInstance();

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

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

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnectionsOpcua)
{
    auto instance = setupBaseInstance();
    configureOpcuaClientInstance(instance);

    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewManufacturer("openDAQ");
    test1Options.setNewSerialNumber("Test3");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewManufacturer("openDAQ");
    test2Options.setNewSerialNumber("Test1");

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices()[0].getLocalId(), "openDAQ_Test3");
    ASSERT_EQ(instance.getDevices()[1].getLocalId(), "openDAQ_Test1");

    ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
    ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              serverInstance3.getSignalsRecursive()[0]);
        
    auto root1 = instance.getDevices()[0];
    auto child1 = root1.getDevices()[0];
    auto root2 = instance.getDevices()[1];
    auto child2 = root2.getDevices()[0];

    ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
    ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
    ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
    ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);
}

TEST_F(RemoteModulesUpdateTest, RemapCheckIPConnectionsNativeNewInstanceOpcua)
{
    auto instance = setupBaseInstance();
    configureOpcuaClientInstance(instance);
    
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewManufacturer("openDAQ");
    test1Options.setNewSerialNumber("Test3");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewManufacturer("openDAQ");
    test2Options.setNewSerialNumber("Test1");

    auto freshInstance = setupBaseInstance();

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

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

TEST_F(RemoteModulesUpdateTest, GatewayDeviceRemapNative)
{
    auto gateway = setupBaseInstance();
    configureNativeClientInstance(gateway);
    addServers(gateway, 4843, -1);

    auto client = setupBaseInstance();
    client.setRootDevice("daqref://device0");
    auto clientFb = client.addFunctionBlock("RefFBModuleScaling");
    addDevice(client, ConnectionType::Native, 4843);

    auto clientSig = client.getChannels()[0].getSignals()[0];
    clientFb.getInputPorts()[0].connect(gateway.getDevices()[0].getSignalsRecursive()[0]);

    auto serializer = JsonSerializer();
    client.asPtr<IUpdatable>().serializeForUpdate(serializer);
    clientFb.getInputPorts()[0].disconnect();

    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto gatewayOptions = rootChildOptions[0];
    auto test1Options = gatewayOptions.getChildDeviceOptions()[0];
    auto test2Options = gatewayOptions.getChildDeviceOptions()[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewManufacturer("openDAQ");
    test1Options.setNewSerialNumber("Test3");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewManufacturer("openDAQ");
    test2Options.setNewSerialNumber("Test1");
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    client.loadConfiguration(str, params);

    ASSERT_EQ(serverInstance1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance1.getDevices()[0].getSignalsRecursive()[0]);
    ASSERT_EQ(serverInstance1.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),serverInstance1.getSignalsRecursive()[0]);
    ASSERT_EQ(serverInstance3.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), serverInstance3.getDevices()[0].getSignalsRecursive()[0]);
    ASSERT_EQ(serverInstance3.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),serverInstance3.getSignalsRecursive()[0]);

    ASSERT_EQ(gateway.getDevices()[0].getLocalId(), "openDAQ_Test3");
    ASSERT_EQ(gateway.getDevices()[1].getLocalId(), "openDAQ_Test1");

    auto root1 = gateway.getDevices()[0];
    auto child1 = root1.getDevices()[0];
    auto root2 = gateway.getDevices()[1];
    auto child2 = root2.getDevices()[0];
    
    ASSERT_EQ(child1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root1.getSignalsRecursive()[0]);
    ASSERT_EQ(root1.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child1.getSignalsRecursive()[0]);
    ASSERT_EQ(child2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), root2.getSignalsRecursive()[0]);
    ASSERT_EQ(root2.getFunctionBlocks()[0].getInputPorts()[0].getSignal(), child2.getSignalsRecursive()[0]);

    auto gatewayClientObj = client.getDevices()[0];
    ASSERT_EQ(gatewayClientObj.getDevices()[0].getLocalId(), "openDAQ_Test1");
    ASSERT_EQ(gatewayClientObj.getDevices()[1].getLocalId(), "openDAQ_Test3");

    clientFb = client.getFunctionBlocks()[0];
    ASSERT_EQ(clientFb.getInputPorts()[0].getSignal(), client.getDevices()[0].getDevices()[1].getSignalsRecursive()[0]);
}

// TODO: Generates duplicate item warning
TEST_F(RemoteModulesUpdateTest, RemapWithConnectionStringNative)
{
    auto instance = setupBaseInstance();
    auto clientFb = instance.addFunctionBlock("RefFBModuleScaling");
    configureNativeClientInstance(instance);

    clientFb.getInputPorts()[0].connect(instance.getDevices()[0].getSignalsRecursive()[0]);
    
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewConnectionString("daq://openDAQ_Test3");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewConnectionString("daq://openDAQ_Test1");

    auto freshInstance = setupBaseInstance();

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

    auto id1 = freshInstance.getDevices()[0].getLocalId();
    auto id2 = freshInstance.getDevices()[1].getLocalId();

    ASSERT_EQ(freshInstance.getDevices()[0].getLocalId(), "openDAQ_Test3");
    ASSERT_EQ(freshInstance.getDevices()[1].getLocalId(), "openDAQ_Test1");

    clientFb = freshInstance.getFunctionBlocks()[0];
    ASSERT_EQ(clientFb.getInputPorts()[0].getSignal(), freshInstance.getDevices()[0].getSignalsRecursive()[0]);
}

TEST_F(RemoteModulesUpdateTest, RemapWithConnectionStringOPCUA)
{
    auto instance = setupBaseInstance();
    auto clientFb = instance.addFunctionBlock("RefFBModuleScaling");
    configureOpcuaClientInstance(instance);

    clientFb.getInputPorts()[0].connect(instance.getDevices()[0].getSignalsRecursive()[0]);
    
    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];

    test1Options.setUpdateMode(DeviceUpdateMode::Remap);
    test1Options.setNewConnectionString("daq.opcua://127.0.0.1:4842");

    test2Options.setUpdateMode(DeviceUpdateMode::Remap);
    test2Options.setNewConnectionString("daq.opcua://127.0.0.1:4840");

    auto freshInstance = setupBaseInstance();

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    freshInstance.loadConfiguration(str, params);

    auto id1 = freshInstance.getDevices()[0].getLocalId();
    auto id2 = freshInstance.getDevices()[1].getLocalId();

    ASSERT_EQ(freshInstance.getDevices()[0].getLocalId(), "openDAQ_Test3");
    ASSERT_EQ(freshInstance.getDevices()[1].getLocalId(), "openDAQ_Test1");

    clientFb = freshInstance.getFunctionBlocks()[0];
    ASSERT_EQ(clientFb.getInputPorts()[0].getSignal(), freshInstance.getDevices()[0].getSignalsRecursive()[0]);
}

TEST_F(RemoteModulesUpdateTest, UpdateOnlyNative)
{
    auto instance = setupBaseInstance();
    configureNativeClientInstance(instance);

    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];
    test1Options.setUpdateMode(DeviceUpdateMode::UpdateOnly);
    test2Options.setUpdateMode(DeviceUpdateMode::UpdateOnly);

    instance.getDevices()[0].setPropertyValue("AcquisitionLoopTime", 50);
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices()[0].getPropertyValue("AcquisitionLoopTime"), 20u);

    auto freshInstance = setupBaseInstance();
    freshInstance.loadConfiguration(str, params);

    ASSERT_EQ(freshInstance.getDevices().getCount(), 0u);
}

TEST_F(RemoteModulesUpdateTest, OptionsSkipNative)
{
    auto instance = setupBaseInstance();
    configureNativeClientInstance(instance);

    auto serializer = JsonSerializer();
    instance.asPtr<IUpdatable>().serializeForUpdate(serializer);
   
    auto str = serializer.getOutput();
    auto options = DeviceUpdateOptions(str);
    auto rootChildOptions = options.getChildDeviceOptions();
    auto test1Options = rootChildOptions[0];
    auto test2Options = rootChildOptions[1];
    test1Options.setUpdateMode(DeviceUpdateMode::Skip);
    test2Options.setUpdateMode(DeviceUpdateMode::Skip);

    instance.getDevices()[0].setPropertyValue("AcquisitionLoopTime", 50);
    
    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);
    instance.loadConfiguration(str, params);

    ASSERT_EQ(instance.getDevices()[0].getPropertyValue("AcquisitionLoopTime"), 50u);

    auto freshInstance = setupBaseInstance();
    freshInstance.loadConfiguration(str, params);

    ASSERT_EQ(freshInstance.getDevices().getCount(), 0u);
}

// TODO: Add DeviceUpdateOptions::Remove test
// TODO: Client-to-device loading is not properly establishing connections.
