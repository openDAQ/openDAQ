#include "test_helpers.h"
#include <gtest/gtest.h>
#include <opendaq/function_block_type_ptr.h>
#include <opendaq/update_parameters_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/module_impl.h>
#include <testutils/testutils.h>

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

TEST_F(InstanceTest, InstanceGetters)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_NO_THROW(instance.getContext());
    ASSERT_NO_THROW(instance.getModuleManager());
}

TEST_F(InstanceTest, GetSetRootDevice)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getRootDevice().getInfo().getName(), String("OpenDAQClient"));
    instance.setRootDevice("daqmock://client_device");
}

TEST_F(InstanceTest, SetRootDeviceWithConfig)
{
    auto instance = test_helpers::setupInstance();

    auto deviceTypes = instance.getAvailableDeviceTypes();
    auto config = deviceTypes.get("mock_phys_device").createDefaultConfig();
    config.setPropertyValue("message", "Hello from config.");

    ASSERT_NO_THROW(instance.setRootDevice("daqmock://phys_device", config));

    auto rootDevice = instance.getRootDevice();
    ASSERT_TRUE(rootDevice.hasProperty("message"));
    ASSERT_EQ(rootDevice.getPropertyValue("message"), config.getPropertyValue("message"));
}

TEST_F(InstanceTest, RootDeviceWithModuleFunctionBlocks)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getRootDevice().getInfo().getName(), String("OpenDAQClient"));
    instance.setRootDevice("daqmock://phys_device");

    auto fbs = instance.getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 0u);

    auto fbTypes = instance.getAvailableFunctionBlockTypes();
    ASSERT_EQ(fbTypes.getCount(), 3u);
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

    ASSERT_THROW(instance.setRootDevice("daqmock://phys_device"), InvalidStateException);

}

// IDevice

TEST_F(InstanceTest, DeviceInformation)
{
    auto instance = test_helpers::setupInstance();
    ASSERT_EQ(instance.getInfo().getName(), String("OpenDAQClient"));
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

    ASSERT_TRUE(deviceTypes.hasKey("mock_client_device"));
    auto mockDevice = deviceTypes.get("mock_client_device");
    ASSERT_EQ(mockDevice.getId(), "mock_client_device");

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
        if (deviceInfo.getConnectionString() != "daqmock://client_device")
            instance.addDevice(deviceInfo.getConnectionString());

    ASSERT_EQ(instance.getDevices().getCount(), 1u);
}

TEST_F(InstanceTest, RemoveDevice)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices.getCount(), 2u);

    for (const auto& deviceInfo : availableDevices)
        if (deviceInfo.getConnectionString() != "daqmock://client_device")
            instance.addDevice(deviceInfo.getConnectionString());

    const auto devices = instance.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    for (const auto& device : devices)
        instance.removeDevice(device);

    ASSERT_EQ(instance.getDevices().getCount(), 0u);
}

TEST_F(InstanceTest, AddDevicesParallelSuccess)
{
    auto instance = test_helpers::setupInstance();

    auto connectionArgs =
        Dict<IString, IPropertyObject>(
            {
                {"daqmock://phys_device", nullptr},
                {"daqmock://client_device", nullptr}
            }
        );
    auto devices = instance.addDevices(connectionArgs);
    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_EQ(devices.get("daqmock://phys_device").getInfo().getConnectionString(), "daqmock://phys_device");
    ASSERT_EQ(devices.get("daqmock://client_device").getInfo().getConnectionString(), "daqmock://client_device");

    ASSERT_EQ(instance.getDevices().getCount(), 2u);
    ASSERT_EQ(instance.getDevices()[0], devices.get("daqmock://phys_device"));
    ASSERT_EQ(instance.getDevices()[1], devices.get("daqmock://client_device"));

    auto nestedDevices = instance.getDevices()[0].addDevices(connectionArgs);
    ASSERT_EQ(nestedDevices.getCount(), 2u);
    ASSERT_EQ(instance.getDevices()[0].getDevices().getCount(), 2u);
}

TEST_F(InstanceTest, AddDevicesParallelPartialSuccess)
{
    auto instance = test_helpers::setupInstance();
    instance.addDevice("daqmock://client_device");

    auto connectionArgs =
        Dict<IString, IPropertyObject>(
            {
                {"daqmock://client_device", nullptr},
                {"daqmock://phys_device", nullptr}
            }
        );
    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();
    DictPtr<IString, IDevice> devices;

    ASSERT_EQ(instance->addDevices(&devices, connectionArgs, errCodes, errorInfos), OPENDAQ_PARTIAL_SUCCESS);

    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_FALSE(devices.get("daqmock://client_device").assigned());
    ASSERT_TRUE(devices.get("daqmock://phys_device").assigned());

    ASSERT_EQ(errCodes.getCount(), 2u);
    ASSERT_EQ(errCodes.get("daqmock://client_device"), OPENDAQ_ERR_DUPLICATEITEM);
    ASSERT_EQ(errCodes.get("daqmock://phys_device"), OPENDAQ_SUCCESS);

    ASSERT_EQ(errorInfos.getCount(), 2u);
    ASSERT_TRUE(errorInfos.get("daqmock://client_device").assigned());
    ASSERT_FALSE(errorInfos.get("daqmock://phys_device").assigned());

    ASSERT_EQ(instance.getDevices().getCount(), 2u);
    ASSERT_EQ(instance.getDevices()[1], devices.get("daqmock://phys_device"));
}

TEST_F(InstanceTest, AddDevicesParallelFailure)
{
    auto instance = test_helpers::setupInstance();
    instance.addDevice("daqmock://client_device");

    ASSERT_THROW_MSG(instance.addDevices(Dict<IString, IPropertyObject>()), InvalidParameterException, "None connection arguments provided");

    auto connectionArgs =
        Dict<IString, IPropertyObject>(
            {
                {"daqmock://client_device", nullptr},
                {"unknown://unknown", nullptr}
            }
        );
    auto errCodes = Dict<IString, IInteger>();
    DictPtr<IString, IDevice> devices;
    ASSERT_THROW_MSG(devices = instance.addDevices(connectionArgs, errCodes), GeneralErrorException, "No devices were added");
    ASSERT_EQ(errCodes.getCount(), 2u);
    ASSERT_EQ(errCodes.get("daqmock://client_device"), OPENDAQ_ERR_DUPLICATEITEM);
    ASSERT_EQ(errCodes.get("unknown://unknown"), OPENDAQ_ERR_NOTFOUND);

    ASSERT_EQ(instance.getDevices().getCount(), 1u);
}

TEST_F(InstanceTest, AddNested)
{
    auto instance = test_helpers::setupInstance();
    auto availableDevices = instance.getAvailableDevices();
    ASSERT_EQ(availableDevices[0].getConnectionString(), "daqmock://client_device");

    DevicePtr device1, device2, device3;
    ASSERT_NO_THROW(device1 = instance.addDevice("daqmock://phys_device"));
    ASSERT_NO_THROW(device2 = device1.addDevice("daqmock://phys_device"));
    ASSERT_NO_THROW(device3 = device2.addDevice("daqmock://phys_device"));
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
    ASSERT_EQ(availableDevices[1].getConnectionString(), "daqmock://phys_device");

    auto device = instance.addDevice("daqmock://phys_device");
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

    ASSERT_TRUE(serverTypes.hasKey("OpenDAQLTStreaming"));
    mockServer = serverTypes.get("OpenDAQLTStreaming");
    ASSERT_EQ(mockServer.getId(), "OpenDAQLTStreaming");

    ASSERT_TRUE(serverTypes.hasKey("OpenDAQNativeStreaming"));
    mockServer = serverTypes.get("OpenDAQNativeStreaming");
    ASSERT_EQ(mockServer.getId(), "OpenDAQNativeStreaming");

    ASSERT_TRUE(serverTypes.hasKey("OpenDAQOPCUA"));
    mockServer = serverTypes.get("OpenDAQOPCUA");
    ASSERT_EQ(mockServer.getId(), "OpenDAQOPCUA");
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
        if (deviceInfo.getConnectionString() != "daqmock://client_device")
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
    const auto authenticationProvider = AuthenticationProvider();

    const auto instanceBuilder = InstanceBuilder()
                                .setLogger(logger)
                                .setGlobalLogLevel(LogLevel::Debug)
                                .setComponentLogLevel("component1", LogLevel::Critical)
                                .setSinkLogLevel(StdOutLoggerSink(), LogLevel::Warn)
                                .setModulePath("./modulePath2")
                                .setModuleManager(moduleManager)
                                .setSchedulerWorkerNum(1)
                                .setScheduler(scheduler)
                                .setDefaultRootDeviceLocalId("OpenDAQClient")
                                .setRootDevice("test")
                                .setDefaultRootDeviceInfo(defaultRootDeviceInfo)
                                .setAuthenticationProvider(authenticationProvider);

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
    ASSERT_EQ(instanceBuilder.getDefaultRootDeviceLocalId(), "OpenDAQClient");
    ASSERT_EQ(instanceBuilder.getRootDevice(), "test");
    ASSERT_EQ(instanceBuilder.getDefaultRootDeviceInfo(), defaultRootDeviceInfo);
    ASSERT_EQ(instanceBuilder.getAuthenticationProvider(), authenticationProvider);
}

TEST_F(InstanceTest, InstanceBuilderSetContext)
{
    const auto logger = Logger();
    const auto moduleManager = ModuleManager("[[none]]");
    const auto typeManager = TypeManager();
    const auto authenticationProvider = AuthenticationProvider();
    const auto context = Context(nullptr, logger, typeManager, moduleManager, authenticationProvider);

    const ModulePtr deviceModule(MockDeviceModule_Create(context));
    moduleManager.addModule(deviceModule);

    auto instanceNoContext = InstanceBuilder().build();
    ASSERT_NE(instanceNoContext.getContext(), context);

    auto instance = InstanceBuilder().setContext(context).build();
    ASSERT_EQ(instance.getContext(), context);
    ASSERT_EQ(instance.getContext().getTypeManager(), typeManager);
    ASSERT_EQ(instance.getContext().getLogger(), logger);
    ASSERT_EQ(instance.getContext().getScheduler(), context.getScheduler());
    ASSERT_EQ(instance.getContext().getAuthenticationProvider(), authenticationProvider);
    ASSERT_NO_THROW(instance.addDevice("daqmock://phys_device"));
}

TEST_F(InstanceTest, InstanceBuilderRootDeviceConfig)
{
    auto config = PropertyObject();
    config.addProperty(StringProperty("message", "Hello from config."));

    const auto moduleManager = ModuleManager("[[none]]");
    const auto context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr);

    const ModulePtr deviceModule(MockDeviceModule_Create(context));
    moduleManager.addModule(deviceModule);

    auto instance = InstanceBuilder().setContext(context).setRootDevice("daqmock://phys_device", config).build();

    auto rootDevice = instance.getRootDevice();
    ASSERT_TRUE(rootDevice.hasProperty("message"));
    ASSERT_EQ(rootDevice.getPropertyValue("message"), config.getPropertyValue("message"));
}

TEST_F(InstanceTest, InstanceCreateFactory)
{
    const auto logger = Logger();
    const auto scheduler = Scheduler(logger, 2);
    const auto moduleManager = ModuleManager("");
    const auto authenticationProvider = AuthenticationProvider();
    const auto defaultRootDeviceInfo = DeviceInfo("daqref://device0");

    auto instance = InstanceBuilder()
                        .setLogger(logger)
                        .setGlobalLogLevel(LogLevel::Debug)
                        .setModuleManager(moduleManager)
                        .setScheduler(scheduler)
                        .setDefaultRootDeviceLocalId("OpenDAQClient")
                        .setDefaultRootDeviceInfo(defaultRootDeviceInfo)
                        .setSchedulerWorkerNum(1)
                        .setAuthenticationProvider(authenticationProvider)
                        .build();

    ASSERT_EQ(instance.getContext().getLogger(), logger);
    ASSERT_EQ(instance.getContext().getLogger().getLevel(), LogLevel::Debug);
    ASSERT_EQ(instance.getContext().getScheduler(), scheduler);
    ASSERT_EQ(instance.getContext().getScheduler().isMultiThreaded(), true);
    ASSERT_EQ(instance.getContext().getModuleManager(), moduleManager);
    ASSERT_EQ(instance.getContext().getAuthenticationProvider(), authenticationProvider);
    ASSERT_EQ(instance.getRootDevice().getName(), String("OpenDAQClient"));

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

    instance.setRootDevice("daqmock://phys_device");
    ASSERT_TRUE(instance.getRootDevice().assigned());
    ASSERT_EQ(instance.getRootDevice().getName(), "MockPhysicalDevice");
}

TEST_F(InstanceTest, AddServerBackwardsCompat)
{
    auto instance = Instance();

    ASSERT_NO_THROW(instance.addServer("openDAQ Native Streaming", nullptr));
    ASSERT_NO_THROW(instance.addServer("openDAQ LT Streaming", nullptr));
    ASSERT_NO_THROW(instance.addServer("openDAQ OpcUa", nullptr));
}

TEST_F(InstanceTest, SaveLoadRestoreDevice)
{
    std::map<std::string, std::string> devicesNames;
    auto instance = test_helpers::setupInstance("localIntanceId");
    devicesNames.emplace(instance.addDevice("daqmock://phys_device").getName(), "daqmock://phys_device");
    devicesNames.emplace(instance.addDevice("daqmock://client_device").getName(), "daqmock://client_device");

    auto config = instance.saveConfiguration();

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.addDevice("daqmock://phys_device");
    instance2.addDevice("daqmock://client_device");
    instance2.loadConfiguration(config);

    ASSERT_EQ(instance2.getDevices().getCount(), devicesNames.size());
    for (const auto& device : instance2.getDevices())
    {
        ASSERT_TRUE(devicesNames.find(device.getName()) != devicesNames.end());
        ASSERT_EQ(devicesNames[device.getName()], device.getInfo().getConnectionString());
        devicesNames.erase(device.getName());
    }
}

TEST_F(InstanceTest, SaveLoadLocked)
{
    std::map<std::string, std::string> devicesNames;
    auto instance = test_helpers::setupInstance("localIntanceId");
    devicesNames.emplace(instance.addDevice("daqmock://phys_device").getName(), "daqmock://phys_device");
    devicesNames.emplace(instance.addDevice("daqmock://client_device").getName(), "daqmock://client_device");

    instance.lock();

    auto config = instance.saveConfiguration();
    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.loadConfiguration(config);

    ASSERT_TRUE(instance2.isLocked());
    ASSERT_EQ(instance2.getDevices().getCount(), devicesNames.size());

    for (const auto& device : instance2.getDevices())
    {
        ASSERT_TRUE(device.isLocked());
        ASSERT_TRUE(devicesNames.find(device.getName()) != devicesNames.end());
        ASSERT_EQ(devicesNames[device.getName()], device.getInfo().getConnectionString());
    }
}

TEST_F(InstanceTest, SaveLoadRestoreDeviceDifferentIds)
{
    std::map<std::string, std::string> devicesNames;
    auto instance = test_helpers::setupInstance("localIntanceId");
    devicesNames.emplace(instance.addDevice("daqmock://phys_device").getName(), "daqmock://phys_device");
    devicesNames.emplace(instance.addDevice("daqmock://client_device").getName(), "daqmock://client_device");

    auto config = instance.saveConfiguration();

    auto instance2 = test_helpers::setupInstance("localIntanceId2");
    instance2.loadConfiguration(config);

    ASSERT_EQ(instance2.getDevices().getCount(), devicesNames.size());
    for (const auto& device : instance2.getDevices())
    {
        ASSERT_TRUE(devicesNames.find(device.getName()) != devicesNames.end());
        ASSERT_EQ(devicesNames[device.getName()], device.getInfo().getConnectionString());
        devicesNames.erase(device.getName());
    }
}

TEST_F(InstanceTest, SaveLoadReadDevice)
{
    std::map<std::string, std::string> devicesNames;
    auto instance = test_helpers::setupInstance("localIntanceId");
    devicesNames.emplace(instance.addDevice("daqmock://phys_device").getName(), "daqmock://phys_device");
    devicesNames.emplace(instance.addDevice("daqmock://client_device").getName(), "daqmock://client_device");

    auto config = instance.saveConfiguration();

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.addDevice("daqmock://phys_device");
    instance2.addDevice("daqmock://client_device");
    auto loadConfig = UpdateParameters();
    loadConfig.setReAddDevicesEnabled(true);
    instance2.loadConfiguration(config, loadConfig);

    ASSERT_EQ(instance2.getDevices().getCount(), devicesNames.size());

    for (const auto& device : instance2.getDevices())
    {
        ASSERT_TRUE(devicesNames.find(device.getName()) != devicesNames.end());
        ASSERT_EQ(devicesNames[device.getName()], device.getInfo().getConnectionString());
        devicesNames.erase(device.getName());
    }
}

TEST_F(InstanceTest, SaveLoadReadDevice2)
{
    std::map<std::string, std::string> devicesNames;
    auto instance = test_helpers::setupInstance("localIntanceId");
    devicesNames.emplace(instance.addDevice("daqmock://phys_device").getName(), "daqmock://phys_device");
    devicesNames.emplace(instance.addDevice("daqmock://client_device").getName(), "daqmock://client_device");

    auto config = instance.saveConfiguration();

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.addDevice("daqmock://phys_device");
    auto loadConfig = UpdateParameters();
    loadConfig.setReAddDevicesEnabled(true);
    instance2.loadConfiguration(config, loadConfig);

    ASSERT_EQ(instance2.getDevices().getCount(), devicesNames.size());

    for (const auto& device : instance2.getDevices())
    {
        ASSERT_TRUE(devicesNames.find(device.getName()) != devicesNames.end());
        ASSERT_EQ(devicesNames[device.getName()], device.getInfo().getConnectionString());
        devicesNames.erase(device.getName());
    }
}

TEST_F(InstanceTest, SaveLoadReadDevice3)
{
    std::map<std::string, std::string> devicesNames;
    auto instance = test_helpers::setupInstance("localIntanceId");
    devicesNames.emplace(instance.addDevice("daqmock://phys_device").getName(), "daqmock://phys_device");
    devicesNames.emplace(instance.addDevice("daqmock://client_device").getName(), "daqmock://client_device");

    auto config = instance.saveConfiguration();

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    auto loadConfig = UpdateParameters();
    loadConfig.setReAddDevicesEnabled(true);
    instance2.loadConfiguration(config, loadConfig);

    ASSERT_EQ(instance2.getDevices().getCount(), devicesNames.size());

    for (const auto& device : instance2.getDevices())
    {
        ASSERT_TRUE(devicesNames.find(device.getName()) != devicesNames.end());
        ASSERT_EQ(devicesNames[device.getName()], device.getInfo().getConnectionString());
        devicesNames.erase(device.getName());
    }
}

TEST_F(InstanceTest, SaveLoadFunctionsOrdered)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");

        auto fb1 = instance.addFunctionBlock("mock_fb_uid");
        auto fb2 = instance.addFunctionBlock("mock_fb_uid");
        fb2.getInputPorts()[0].connect(fb1.getSignals()[0]);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 2u);

    FunctionBlockPtr restoredFb2;
    for (const auto & fb : restoredFbs)
    {
        if (fb.getLocalId() == "mock_fb_uid_2")
        {
            restoredFb2 = fb;
            break;
        }
    }
    ASSERT_EQ(restoredFb2.getLocalId(), "mock_fb_uid_2");
    auto inputSignal = restoredFb2.getInputPorts()[0].getSignal();
    ASSERT_TRUE(inputSignal.assigned());
    ASSERT_EQ(inputSignal.getGlobalId(), "/localIntanceId/FB/mock_fb_uid_1/Sig/UniqueId_1");
}

TEST_F(InstanceTest, SaveLoadFunctionsCircleDependcies)
{
    StringPtr config;
    auto connections = Dict<IString, IString>();
    {
        auto instance = test_helpers::setupInstance("localIntanceId");

        auto fb1 = instance.addFunctionBlock("mock_fb_uid");
        auto fb2 = instance.addFunctionBlock("mock_fb_uid");
        auto fb3 = instance.addFunctionBlock("mock_fb_uid");
        fb2.getInputPorts()[0].connect(fb1.getSignals()[0]);
        fb3.getInputPorts()[0].connect(fb2.getSignals()[0]);
        fb1.getInputPorts()[0].connect(fb3.getSignals()[0]);

        connections.set(fb1.getGlobalId(), fb1.getInputPorts()[0].getSignal().getGlobalId());
        connections.set(fb2.getGlobalId(), fb2.getInputPorts()[0].getSignal().getGlobalId());
        connections.set(fb3.getGlobalId(), fb3.getInputPorts()[0].getSignal().getGlobalId());

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 3u);
    // yes, we are still attemping to restore all connections
    for (const auto & fb : restoredFbs)
    {
        ASSERT_TRUE(connections.hasKey(fb.getGlobalId()));
        ASSERT_EQ(connections.get(fb.getGlobalId()), fb.getInputPorts()[0].getSignal().getGlobalId());
    }
}

TEST_F(InstanceTest, SaveLoadFunctionsOrderedDifferentIds)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");

        auto fb1 = instance.addFunctionBlock("mock_fb_uid");
        auto fb2 = instance.addFunctionBlock("mock_fb_uid");
        fb2.getInputPorts()[0].connect(fb1.getSignals()[0]);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId2");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 2u);
    FunctionBlockPtr restoredFb2;
    for (const auto & fb : restoredFbs)
    {
        if (fb.getLocalId() == "mock_fb_uid_2")
        {
            restoredFb2 = fb;
            break;
        }
    }
    auto inputSignal = restoredFb2.getInputPorts()[0].getSignal();
    ASSERT_TRUE(!inputSignal.assigned());
    // ASSERT_EQ(inputSignal.getGlobalId(), "/localIntanceId2/FB/mock_fb_uid_1/Sig/UniqueId_1");
}

TEST_F(InstanceTest, SaveLoadFunctionsUnordered)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");

        auto fb1 = instance.addFunctionBlock("mock_fb_uid");
        auto fb2 = instance.addFunctionBlock("mock_fb_uid");
        fb1.getInputPorts()[0].connect(fb2.getSignals()[0]);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.loadConfiguration(config);

    config = instance2.saveConfiguration();

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 2u);
    FunctionBlockPtr restoredFb1;
    for (const auto & fb : restoredFbs)
    {
        if (fb.getLocalId() == "mock_fb_uid_1")
        {
            restoredFb1 = fb;
            break;
        }
    }
    ASSERT_EQ(restoredFb1.getLocalId(), "mock_fb_uid_1");
    auto inputSignal = restoredFb1.getInputPorts()[0].getSignal();
    ASSERT_TRUE(inputSignal.assigned());
    ASSERT_EQ(inputSignal.getGlobalId(), "/localIntanceId/FB/mock_fb_uid_2/Sig/UniqueId_1");
}

TEST_F(InstanceTest, SaveLoadFunctionConnectingDynamicPorts)
{
    auto connections = Dict<IString, IString>();
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");
        auto fb1 = instance.addFunctionBlock("mock_fb_uid");
        auto fb2 = instance.addFunctionBlock("mock_fb_dynamic_output_ports_uid");
        auto fb3 = instance.addFunctionBlock("mock_fb_dynamic_input_ports_uid");
        auto fb4 = instance.addFunctionBlock("mock_fb_uid");
        
        fb4.getInputPorts()[0].connect(fb1.getSignals()[0]);
        fb3.getInputPorts()[0].connect(fb4.getSignals()[0]);
        fb2.getInputPorts()[0].connect(fb3.getSignals()[0]);
        fb1.getInputPorts()[0].connect(fb2.getSignals()[0]);
       
        connections.set(fb1.getGlobalId(), fb1.getInputPorts()[0].getSignal().getGlobalId());
        connections.set(fb2.getGlobalId(), fb2.getInputPorts()[0].getSignal().getGlobalId());
        connections.set(fb3.getGlobalId(), fb3.getInputPorts()[0].getSignal().getGlobalId());
        connections.set(fb4.getGlobalId(), fb4.getInputPorts()[0].getSignal().getGlobalId());

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 4u);

    for (const auto & fb : restoredFbs)
    {
        ASSERT_TRUE(connections.hasKey(fb.getGlobalId()));
        ASSERT_EQ(connections.get(fb.getGlobalId()), fb.getInputPorts()[0].getSignal().getGlobalId());
    }
    
}

TEST_F(InstanceTest, SaveLoadFunctionConnectingSignalFromDev)
{
    StringPtr config;
    StringPtr signalId;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");
        auto fb = instance.addFunctionBlock("mock_fb_uid");
        auto dev = instance.addDevice("daqmock://phys_device");
        auto sig = dev.getSignalsRecursive()[0];
        signalId = sig.getGlobalId();
        fb.getInputPorts()[0].connect(sig);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId");
    instance2.loadConfiguration(config);

    auto restoredDevs = instance2.getDevices();
    ASSERT_EQ(restoredDevs.getCount(), 1u);
    auto restoredDev = restoredDevs[0];

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 1u);
    auto restoredFb = restoredFbs[0];

    auto restoredSig = restoredFb.getInputPorts()[0].getSignal();
    ASSERT_TRUE(restoredSig.assigned());
    ASSERT_EQ(restoredSig.getGlobalId(), signalId);
}

TEST_F(InstanceTest, SaveLoadFunctionNeastedFb)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");
        auto fb = instance.addFunctionBlock("mock_fb_uid");
        fb.addFunctionBlock("NestedFBId");
        fb.addFunctionBlock("NestedFBId");
        ASSERT_EQ(fb.getFunctionBlocks().getCount(), 3u);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId2");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 1u);
    auto restoredFb = restoredFbs[0];

    auto nestedFbs = restoredFb.getFunctionBlocks();
    ASSERT_EQ(nestedFbs.getCount(), 3u);
}

TEST_F(InstanceTest, SaveLoadFunctionNeastedFb2)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");
        auto fb = instance.addFunctionBlock("mock_fb_uid");
        fb.addFunctionBlock("NestedFBId");
        fb.addFunctionBlock("NestedFBId");
        ASSERT_EQ(fb.getFunctionBlocks().getCount(), 3u);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId2");
    auto fb = instance2.addFunctionBlock("mock_fb_uid");
    fb.addFunctionBlock("NestedFBId");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 1u);
    auto restoredFb = restoredFbs[0];

    auto nestedFbs = restoredFb.getFunctionBlocks();
    ASSERT_EQ(nestedFbs.getCount(), 3u);
}

TEST_F(InstanceTest, SaveLoadFunctionNeastedFb3)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance("localIntanceId");
        auto fb = instance.addFunctionBlock("mock_fb_uid");
        fb.addFunctionBlock("NestedFBId");
        fb.addFunctionBlock("NestedFBId");
        ASSERT_EQ(fb.getFunctionBlocks().getCount(), 3u);

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance("localIntanceId2");
    auto fb = instance2.addFunctionBlock("mock_fb_uid");
    fb.addFunctionBlock("NestedFBId");
    fb.addFunctionBlock("NestedFBId");
    instance2.loadConfiguration(config);

    auto restoredFbs = instance2.getFunctionBlocks();
    ASSERT_EQ(restoredFbs.getCount(), 1u);
    auto restoredFb = restoredFbs[0];

    auto nestedFbs = restoredFb.getFunctionBlocks();
    ASSERT_EQ(nestedFbs.getCount(), 3u);
}

TEST_F(InstanceTest, DISABLED_SaveLoadServers)
{
    StringPtr config;
    StringPtr serverId;
    {
        auto instance = Instance();
        auto server = instance.addServer("OpenDAQOPCUA", nullptr);
        serverId = server.getId();
        config = instance.saveConfiguration();
    }

    auto instance2 = Instance();
    instance2.loadConfiguration(config);

    auto servers = instance2.getServers();
    ASSERT_EQ(servers.getCount(), 1u);
    ASSERT_EQ(servers[0].getId(), serverId);
}

TEST_F(InstanceTest, SaveLoadDeviceConfigOld)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance();

        auto deviceConfig = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = deviceConfig.getPropertyValue("General");
        general.setPropertyValue("StreamingConnectionHeuristic", 2);

        instance.addDevice("daqmock://phys_device", deviceConfig);
        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance();
    instance2.loadConfiguration(config);

    auto device = instance2.getDevices()[0];
    const auto deviceConfig = device.asPtr<IDevicePrivate>(true).getDeviceConfig();
    ASSERT_TRUE(deviceConfig.assigned());
    ASSERT_TRUE(deviceConfig.hasProperty("General"));

    PropertyObjectPtr general = deviceConfig.getPropertyValue("General");
    ASSERT_EQ(general.getPropertyValue("StreamingConnectionHeuristic"), 2);
}

TEST_F(InstanceTest, SaveLoadDeviceConfig)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance();

        auto deviceConfig = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = deviceConfig.getPropertyValue("General");
        general.setPropertyValue("StreamingConnectionHeuristic", 2);

        instance.addDevice("daqmock://phys_device", deviceConfig);
        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance();
    instance2.loadConfiguration(config);

    auto device = instance2.getDevices()[0];
    const auto deviceConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(deviceConfig.assigned());
    ASSERT_TRUE(deviceConfig.hasProperty("General"));

    PropertyObjectPtr general = deviceConfig.getPropertyValue("General");
    ASSERT_EQ(general.getPropertyValue("StreamingConnectionHeuristic"), 2);
}

TEST_F(InstanceTest, SaveLoadFunctionBlockConfig)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance();

        auto fbConfig = PropertyObject();
        fbConfig.addProperty(IntProperty("TestConfigInt", 100));
        fbConfig.addProperty(StringProperty("TestConfigString", "NewStringValue"));

        instance.addFunctionBlock("mock_fb_uid", fbConfig);
        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance();
    instance2.loadConfiguration(config);

    auto fbs = instance2.getFunctionBlocks();
    ASSERT_TRUE(fbs.getCount() == 1);
        
    auto fb = fbs[0];
    ASSERT_EQ(fb.getPropertyValue("TestConfigInt"), 100);
    ASSERT_EQ(fb.getPropertyValue("TestConfigString"), "NewStringValue");
}

TEST_F(InstanceTest, SaveLoadDeviceStruct)
{
    StringPtr config;
    StringPtr serverId;
    {
        auto instance = test_helpers::setupInstance();
        instance.addDevice("daqmock://phys_device");

        auto device = instance.getDevices()[0];
        StructPtr deviceStruct = device.getPropertyValue("DeviceStructure");
        deviceStruct = StructBuilder(deviceStruct).set("value1", 5e-7).build();
        device.setPropertyValue("DeviceStructure", deviceStruct);

        config = instance.saveConfiguration();
    }

    auto instance = test_helpers::setupInstance();
    instance.loadConfiguration(config);
    auto device = instance.getDevices()[0];
    StructPtr deviceStruct = device.getPropertyValue("DeviceStructure");
    ASSERT_EQ(deviceStruct.get("value1"), 5e-7);
    ASSERT_EQ(deviceStruct.get("value2"), 0.0);
}

TEST_F(InstanceTest, SaveLoadDeviceInfo)
{
    StringPtr config;
    {
        auto instance = test_helpers::setupInstance();
        auto device = instance.addDevice("daqmock://phys_device");
        auto deviceInfo = device.getInfo();
        deviceInfo.setPropertyValue("userName", "testUser");
        deviceInfo.setPropertyValue("location", "testLocation");
        deviceInfo.setPropertyValue("TestChangeableField", "testValue");
        deviceInfo.addProperty(StringProperty("CustomProperty", "defaultValue"));
        deviceInfo.setPropertyValue("CustomProperty", "newValue");

        config = instance.saveConfiguration();
    }

    auto instance2 = test_helpers::setupInstance();
    instance2.loadConfiguration(config);

    auto devices = instance2.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto device = devices[0];
    auto deviceInfo = device.getInfo();
    ASSERT_EQ(deviceInfo.getPropertyValue("userName"), "testUser");
    ASSERT_EQ(deviceInfo.getPropertyValue("location"), "testLocation");
    ASSERT_EQ(deviceInfo.getPropertyValue("TestChangeableField"), "testValue");
    ASSERT_TRUE(deviceInfo.hasProperty("CustomProperty"));
    ASSERT_EQ(deviceInfo.getProperty("CustomProperty").getDefaultValue(), "defaultValue");
    ASSERT_EQ(deviceInfo.getPropertyValue("CustomProperty"), "newValue");
}

TEST_F(InstanceTest, TestRemoved1)
{
    auto instance = test_helpers::setupInstance();
    instance.addFunctionBlock("mock_fb_uid");
    instance.addDevice("daqmock://client_device");
    instance.addDevice("daqmock://phys_device");

    const ListPtr<IComponent> components = instance.getItems(search::Recursive(search::Any()));

    const auto root = instance.getRootDevice();
    root.remove();

    Bool removed = false;
    root.asPtr<IRemovable>()->isRemoved(&removed);
    ASSERT_TRUE(removed);

    for (const auto& component : components)
    {
        component.asPtr<IRemovable>()->isRemoved(&removed);
        ASSERT_TRUE(removed);
    }
}

TEST_F(InstanceTest, TestRemoved2)
{
    auto instance = test_helpers::setupInstance();
    instance.addFunctionBlock("mock_fb_uid");
    instance.addDevice("daqmock://client_device");
    instance.addDevice("daqmock://phys_device");

    ListPtr<IComponent> components = instance.getItems(search::Recursive(search::Any()));
    components.pushBack(instance.getRootDevice());

    instance.release();

    Bool removed = false;
    for (const auto& component : components)
    {
        component.asPtr<IRemovable>()->isRemoved(&removed);
        ASSERT_TRUE(removed);
    }
}

class MockClientModule : public Module
{
public:

    MockClientModule(daq::ContextPtr ctx)
        : Module("Mock Client",
                 daq::VersionInfo(1, 0, 0),
                 std::move(ctx),
                 "MockClient")
    {
    }

    ListPtr<IDeviceInfo> onGetAvailableDevices() override
    {
        const auto info = DeviceInfo("false://mock_device");
        info.setManufacturer("openDAQ");
        info.setSerialNumber("mock_phys_ser");
        const auto cap = ServerCapability("mock", "mock", ProtocolType::ConfigurationAndStreaming);
        info.asPtr<IDeviceInfoInternal>().addServerCapability(cap);
        return List<IDeviceInfo>(info);
    }
};

TEST_F(InstanceTest, ModuleManagerGrouping)
{
    const auto instance = Instance("[[none]]");
    const ModuleManagerPtr moduleManager = instance.getContext().getModuleManager();
    moduleManager.addModule(createWithImplementation<IModule, MockClientModule>(instance.getContext()));
    moduleManager.addModule(MockDeviceModule_Create(instance.getContext()));

    const auto devs = instance.getAvailableDevices();
    ASSERT_EQ(devs.getCount(), 3);

    for (const auto& dev : devs)
    {
        if (dev.getSerialNumber() == "mock_phys_ser")
        {
            if (dev.getServerCapabilities().getCount())
                ASSERT_EQ(dev.getConnectionString(), "daq://openDAQ_mock_phys_ser");
            else
                ASSERT_EQ(dev.getConnectionString(), "daqmock://phys_device");
        }
    }
}

END_NAMESPACE_OPENDAQ
