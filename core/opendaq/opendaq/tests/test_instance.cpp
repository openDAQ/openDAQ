#include "test_helpers.h"
#include <gtest/gtest.h>
#include <opendaq/function_block_type_ptr.h>

using InstanceTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(InstanceTest, Create)
{
    auto instance = test_helpers::setupInstance();
}

TEST_F(InstanceTest, Id)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_TRUE(instance.getLocalId().assigned());
    ASSERT_GT(instance.getLocalId().getLength(), 0u);
    ASSERT_GT(instance.getLocalId().getLength(), 0u);
}

TEST_F(InstanceTest, CustomLocalId)
{
    auto instance = test_helpers::setupInstance("myId");
    ASSERT_EQ(instance.getLocalId(), "myId");
    ASSERT_EQ(instance.getGlobalId(), "/myId");
}

#ifdef _MSC_VER

TEST_F(InstanceTest, LocalIdFromEnvVar)
{
    _putenv("OPENDAQ_INSTANCE_ID=myId");
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getLocalId(), "myId");
    ASSERT_EQ(instance.getGlobalId(), "/myId");
}

#endif

TEST_F(InstanceTest, InstanceGetters)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_NO_THROW(instance.getContext());
    ASSERT_NO_THROW(instance.getModuleManager());
}

TEST_F(InstanceTest, GetSetRootDevice)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getRootDevice().getInfo().getName(), String("daq_client"));
    instance.setRootDevice("daq_client_device");
}

TEST_F(InstanceTest, RootDeviceWithModuleFunctionBlocks)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getRootDevice().getInfo().getName(), String("daq_client"));
    instance.setRootDevice("mock_phys_device");

    auto fbs = instance.getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 0u);

    auto fbTypes = instance.getAvailableFunctionBlockTypes();
    ASSERT_EQ(fbTypes.getCount(), 1u);
    ASSERT_TRUE(fbTypes.hasKey("mock_fb_uid"));
    ASSERT_EQ(fbTypes.get("mock_fb_uid").getId(), "mock_fb_uid");

    auto fb = instance.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "mock_fb_uid");

    fbs = instance.getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    ASSERT_EQ(fb, fbs[0]);

    auto sig = fb.getSignals()[0];
    ASSERT_EQ(sig.getGlobalId(), "/mockdev/FB/mock_fb_uid_1/Sig/UniqueId_1");

    instance.removeFunctionBlock(fb);
    fbs = instance.getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 0u);

    ASSERT_THROW(instance.setRootDevice("mock_phys_device"), InvalidStateException);

}

// IDevice

TEST_F(InstanceTest, DeviceInformation)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getInfo().getName(), String("daq_client"));
}

TEST_F(InstanceTest, DeviceSignals)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getSignals().getCount(), static_cast<SizeT>(0));
}

TEST_F(InstanceTest, ConnectionString)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();

    for (const auto& deviceInfo : availableDevices)
        ASSERT_TRUE(deviceInfo.getConnectionString().assigned());
}

TEST_F(InstanceTest, EnumerateDeviceTypes)
{
    auto instance = test_helpers::setupInstance();
    auto deviceTypes = instance.getAvailableDeviceTypes();
    ASSERT_EQ(deviceTypes.getCount(), 2u);

    ASSERT_TRUE(deviceTypes.hasKey("daq_client_device"));
    auto mockDevice = deviceTypes.get("daq_client_device");
    ASSERT_EQ(mockDevice.getId(), "daq_client_device");

    ASSERT_TRUE(deviceTypes.hasKey("mock_phys_device"));
    mockDevice = deviceTypes.get("mock_phys_device");
    ASSERT_EQ(mockDevice.getId(), "mock_phys_device");
}

TEST_F(InstanceTest, AddDevice)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices.getCount(), 2u);

    for (const auto& deviceInfo : availableDevices)
        if (deviceInfo.getConnectionString() != "daq_client_device")
            instance.addDevice(deviceInfo.getConnectionString());

    ASSERT_EQ(instance.getDevices().getCount(), 1u);
}

TEST_F(InstanceTest, RemoveDevice)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices.getCount(), 2u);
    
    for (const auto& deviceInfo : availableDevices)
        if (deviceInfo.getConnectionString() != "daq_client_device")
            instance.addDevice(deviceInfo.getConnectionString());

    const auto devices = instance.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    for (const auto& device : devices)
        instance.removeDevice(device);

    ASSERT_EQ(instance.getDevices().getCount(), 0u);
}

TEST_F(InstanceTest, AddNested)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices[0].getConnectionString(), "daq_client_device");

    DevicePtr device1, device2, device3;
    ASSERT_NO_THROW(device1 = instance.addDevice("mock_phys_device"));
    ASSERT_NO_THROW(device2 = device1.addDevice("mock_phys_device"));
    ASSERT_NO_THROW(device3 = device2.addDevice("mock_phys_device"));
}

TEST_F(InstanceTest, AddFunctionBlock)
{
    auto instance = test_helpers::setupInstance();
    auto availableFbs = instance.getAvailableFunctionBlockTypes();

    ASSERT_TRUE(availableFbs.hasKey("mock_fb_uid"));
    auto fb1 = instance.addFunctionBlock("mock_fb_uid");
    auto fb2 = instance.addFunctionBlock("mock_fb_uid");
    auto fb3 = instance.addFunctionBlock("mock_fb_uid");

    ASSERT_EQ(instance.getFunctionBlocks().getCount(), 3u);
}

TEST_F(InstanceTest, RemoveFunctionBlock)
{
    auto instance = test_helpers::setupInstance();
    auto availableFbs = instance.getAvailableFunctionBlockTypes();

    ASSERT_TRUE(availableFbs.hasKey("mock_fb_uid"));
    auto fb1 = instance.addFunctionBlock("mock_fb_uid");
    auto fb2 = instance.addFunctionBlock("mock_fb_uid");
    auto fb3 = instance.addFunctionBlock("mock_fb_uid");

    instance.removeFunctionBlock(fb2);
    instance.removeFunctionBlock(fb3);
    instance.removeFunctionBlock(fb1);

    ASSERT_EQ(instance.getFunctionBlocks().getCount(), static_cast<SizeT>(0));
}

TEST_F(InstanceTest, AddFunctionBlockLocalIds)
{
    auto instance = test_helpers::setupInstance();
    auto availableFbs = instance.getAvailableFunctionBlockTypes();

    ASSERT_TRUE(availableFbs.hasKey("mock_fb_uid"));

    auto fb1 = instance.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(fb1.getLocalId(), "mock_fb_uid_1");
    auto fb2 = instance.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(fb2.getLocalId(), "mock_fb_uid_2");
    auto fb3 = instance.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(fb3.getLocalId(), "mock_fb_uid_3");
    auto fb4 = instance.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(fb4.getLocalId(), "mock_fb_uid_4");

    instance.removeFunctionBlock(fb1);
    instance.removeFunctionBlock(fb2);
    instance.removeFunctionBlock(fb4);

    auto fb5 = instance.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(fb5.getLocalId(), "mock_fb_uid_4");

    ASSERT_EQ(instance.getFunctionBlocks().getCount(), 2u);
}

TEST_F(InstanceTest, GetChannels)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices[1].getConnectionString(), "mock_phys_device");

    auto device = instance.addDevice("mock_phys_device");
    ASSERT_EQ(device.getChannels().getCount(), 4u);
}

TEST_F(InstanceTest, EnumerateServerTypes)
{
    auto instance = test_helpers::setupInstance();
    auto serverTypes = instance.getAvailableServerTypes();
    ASSERT_EQ(serverTypes.getCount(), 4u);

    ASSERT_TRUE(serverTypes.hasKey("MockServer"));
    auto mockServer = serverTypes.get("MockServer");
    ASSERT_EQ(mockServer.getId(), "MockServer");

    ASSERT_TRUE(serverTypes.hasKey("openDAQ WebsocketTcp Streaming"));
    mockServer = serverTypes.get("openDAQ WebsocketTcp Streaming");
    ASSERT_EQ(mockServer.getId(), "openDAQ WebsocketTcp Streaming");

    ASSERT_TRUE(serverTypes.hasKey("openDAQ Native Streaming"));
    mockServer = serverTypes.get("openDAQ Native Streaming");
    ASSERT_EQ(mockServer.getId(), "openDAQ Native Streaming");

    ASSERT_TRUE(serverTypes.hasKey("openDAQ OpcUa"));
    mockServer = serverTypes.get("openDAQ OpcUa");
    ASSERT_EQ(mockServer.getId(), "openDAQ OpcUa");
}

TEST_F(InstanceTest, AddServer)
{
    auto instance = test_helpers::setupInstance();
    auto server = instance.addServer("MockServer", nullptr);
    ASSERT_NE(server, nullptr);
}

TEST_F(InstanceTest, GetServers)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getServers().getCount(), 0u);

    auto server = instance.addServer("MockServer", nullptr);
    ASSERT_EQ(instance.getServers().getCount(), 1u);
    ASSERT_EQ(instance.getServers()[0], server);
}

TEST_F(InstanceTest, AddStandardServers)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getServers().getCount(), 0u);

    auto servers = instance.addStandardServers();
    size_t serverCount = 1;
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    serverCount++;
#elif defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    serverCount++;
#endif

    ASSERT_EQ(instance.getServers().getCount(), serverCount);
    ASSERT_EQ(servers.getCount(), serverCount);
    ASSERT_EQ(instance.getServers(), servers);
}

TEST_F(InstanceTest, RemoveServer)
{
    auto instance = test_helpers::setupInstance();
    auto server = instance.addServer("MockServer", nullptr);
    assert(instance.getServers().getCount() == 1u);

    ASSERT_NO_THROW(instance.removeServer(server));
    ASSERT_EQ(instance.getServers().getCount(), 0u);
}

// IPropertyObject

TEST_F(InstanceTest, GetSetProperty)
{
    auto instance = test_helpers::setupInstance();
    auto rootDevice = instance.getRootDevice();

    auto info = PropertyBuilder("test").setValueType(ctInt).setDefaultValue(1).build();
    instance.addProperty(info);

    ASSERT_EQ(rootDevice.getPropertyValue("test"), 1);
    instance.setPropertyValue("test", 2);
    ASSERT_EQ(rootDevice.getPropertyValue("test"), 2);

    rootDevice.setPropertyValue("test", 10);
    ASSERT_EQ(instance.getPropertyValue("test"), 10);
}

TEST_F(InstanceTest, Serialize)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices.getCount(), 2u);

    for (const auto& deviceInfo : availableDevices)
        if (deviceInfo.getConnectionString() != "daq_client_device")
            instance.addDevice(deviceInfo.getConnectionString());

    auto serializer = JsonSerializer(True);

    instance.serialize(serializer);

    auto str = serializer.getOutput();
    std::cout << str << std::endl;
}

TEST_F(InstanceTest, InstanceBuilderSetGet)
{
    const auto logger = Logger();
    const auto scheduler = Scheduler(logger);
    const auto moduleManager = ModuleManager("./modulePath1");
    const auto defaultRootDeviceInfo = DeviceInfo("daqref://device0");

    const auto instanceBuilder = InstanceBuilder()
                                .setLogger(logger)
                                .setGlobalLogLevel(LogLevel::Debug)
                                .setComponentLogLevel("component1", LogLevel::Critical)
                                .setSinkLogLevel(StdOutLoggerSink(), LogLevel::Warn)
                                .setModulePath("./modulePath2")
                                .setModuleManager(moduleManager)
                                .setSchedulerWorkerNum(1)
                                .setScheduler(scheduler)
                                .setDefaultRootDeviceLocalId("DefaultRootDeviceLocalId")
                                .setRootDevice("test")
                                .setDefaultRootDeviceInfo(defaultRootDeviceInfo);
    
    ASSERT_EQ(instanceBuilder.getLogger(), logger);
    ASSERT_EQ(instanceBuilder.getGlobalLogLevel(), LogLevel::Debug);
    
    auto components = instanceBuilder.getComponentsLogLevel();
    ASSERT_EQ(components.getCount(), 1u);
    ASSERT_EQ(components["component1"], LogLevel::Critical);
    
    auto sinks = instanceBuilder.getLoggerSinks();
    ASSERT_EQ(sinks.getCount(), 1u);
    ASSERT_EQ(sinks[0].getLevel(), LogLevel::Warn);

    ASSERT_EQ(instanceBuilder.getModulePath(), "./modulePath2");
    ASSERT_EQ(instanceBuilder.getModuleManager(), moduleManager);
    ASSERT_EQ(instanceBuilder.getSchedulerWorkerNum(), 1u);
    ASSERT_EQ(instanceBuilder.getScheduler(), scheduler);
    ASSERT_EQ(instanceBuilder.getDefaultRootDeviceLocalId(), "DefaultRootDeviceLocalId");
    ASSERT_EQ(instanceBuilder.getRootDevice(), "test");
    ASSERT_EQ(instanceBuilder.getDefaultRootDeviceInfo(), defaultRootDeviceInfo);
}

TEST_F(InstanceTest, InstanceCreateFactory)
{
    const auto logger = Logger();
    const auto scheduler = Scheduler(logger, 2);
    const auto moduleManager = ModuleManager("");
    const auto defaultRootDeviceInfo = DeviceInfo("daqref://device0");

    auto instance = InstanceBuilder()
                                .setLogger(logger)
                                .setGlobalLogLevel(LogLevel::Debug)
                                .setModuleManager(moduleManager)
                                .setScheduler(scheduler)
                                .setDefaultRootDeviceLocalId("DefaultRootDeviceLocalId")
                                .setDefaultRootDeviceInfo(defaultRootDeviceInfo)
                                .setSchedulerWorkerNum(1)
                                .build();

    ASSERT_EQ(instance.getContext().getLogger(), logger);
    ASSERT_EQ(instance.getContext().getLogger().getLevel(), LogLevel::Debug);
    ASSERT_EQ(instance.getContext().getScheduler(), scheduler);
    ASSERT_EQ(instance.getContext().getScheduler().isMultiThreaded(), true);
    ASSERT_EQ(instance.getContext().getModuleManager(), moduleManager);
    ASSERT_EQ(instance.getRootDevice().getName(), "DefaultRootDeviceLocalId"); 

    ASSERT_EQ(instance.getInfo(), defaultRootDeviceInfo);
}

TEST_F(InstanceTest, InstanceBuilderGetDefault)
{
    const auto instanceBuilder = InstanceBuilder()
                                     .setGlobalLogLevel(LogLevel::Debug)
                                     .setComponentLogLevel("Instance", LogLevel::Error)
                                     .addLoggerSink(StdOutLoggerSink())
                                     .setSchedulerWorkerNum(1);

    ASSERT_EQ(instanceBuilder.getLogger().assigned(), false);
    ASSERT_EQ(instanceBuilder.getScheduler().assigned(), false);
    ASSERT_EQ(instanceBuilder.getModuleManager().assigned(), false);

    const auto instance = instanceBuilder.build();

    // check logger
    auto logger = instance.getContext().getLogger();
    ASSERT_EQ(logger.assigned(), true);
    ASSERT_EQ(logger.getLevel(), LogLevel::Debug);
    const auto loggerComponent = logger.getComponent("Instance");
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Error);

    // check scheduler
    auto scheduler = instance.getContext().getScheduler();
    ASSERT_EQ(scheduler.assigned(), true);
    ASSERT_EQ(scheduler.isMultiThreaded(), false);

    // check moduleManager
    ModuleManagerPtr moduleManager = instance.getContext().getModuleManager();
    ASSERT_EQ(moduleManager.assigned(), true);

    // We cannot add modules in the instance builder as the context is not yet created
    const ModulePtr deviceModule(MockDeviceModule_Create(instance.getContext()));
    moduleManager.addModule(deviceModule);

    instance.setRootDevice("mock_phys_device");
    ASSERT_TRUE(instance.getRootDevice().assigned());
    ASSERT_EQ(instance.getRootDevice().getName(), "mockdev");
}

END_NAMESPACE_OPENDAQ
