#include <opendaq/instance_factory.h>
#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opcuatms_client/tms_client.h>
#include <opcuatms_server/tms_server.h>
#include <opendaq/search_filter_factory.h>
#include <chrono>
#include <thread>
#include <testutils/test_comparators.h>
#include <coreobjects/permissions_builder_factory.h>

using namespace daq;
using namespace daq::opcua;
using namespace std::chrono_literals;

class TmsIntegrationTest : public testing::Test
{
public:
    const std::string OPC_URL = "opc.tcp://localhost/";

    InstancePtr createDevice(const StringPtr& localId = "localInstance")
    {
        const auto moduleManager = ModuleManager("[[none]]");
        auto logger = Logger();
        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager, nullptr);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        auto instance = InstanceCustom(context, localId);
        instance.addDevice("daq_client_device");
        instance.addDevice("mock_phys_device");
        instance.addFunctionBlock("mock_fb_uid");

        return instance;
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(TmsIntegrationTest, Connect)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    DevicePtr clientDevice;
    ASSERT_NO_THROW(clientDevice = tmsClient.connect());
    ASSERT_TRUE(clientDevice.assigned());
}

TEST_F(TmsIntegrationTest, Devices)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    DevicePtr clientDevice = tmsClient.connect();
    auto devices = clientDevice.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);
}

TEST_F(TmsIntegrationTest, DeviceInfo)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    DevicePtr clientDevice = tmsClient.connect();

    auto devices = clientDevice.getDevices();
    DeviceInfoPtr deviceInfo;
    ASSERT_NO_THROW(deviceInfo = devices[0].getInfo());
}

TEST_F(TmsIntegrationTest, FunctionBlocks)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    DevicePtr clientDevice = tmsClient.connect();

    auto functionBlocks = clientDevice.getFunctionBlocks();
    ASSERT_EQ(functionBlocks.getCount(), 1u);
}

TEST_F(TmsIntegrationTest, FunctionBlockType)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    DevicePtr clientDevice = tmsClient.connect();

    auto functionBlocks = clientDevice.getFunctionBlocks();
    FunctionBlockTypePtr type;
    ASSERT_NO_THROW(type = functionBlocks[0].getFunctionBlockType());
}

TEST_F(TmsIntegrationTest, GetSignals)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    DevicePtr clientDevice = tmsClient.connect();

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientDevice.getSignals(search::Recursive(search::Visible())));
    // one private signal in MockFunctionBlockImpl. and one in MockPhysicalDeviceImpl
    ASSERT_EQ(signals.getCount(), device.getSignals(search::Recursive(search::Visible())).getCount() - 2);

    ASSERT_NO_THROW(signals = clientDevice.getSignals());
    ASSERT_EQ(signals.getCount(), 0u);
}

TEST_F(TmsIntegrationTest, GetChannels)
{
    InstancePtr instance = createDevice();
    auto devices = instance.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);

    auto device1 = devices.getItemAt(0);
    ASSERT_EQ(device1.getChannels().getCount(), 0u);

    auto device2 = devices.getItemAt(1);
    ASSERT_EQ(device2.getChannels().getCount(), 4u);

    ASSERT_EQ(instance.getChannels(search::Recursive(search::Visible())).getCount(), 4u);
    ASSERT_EQ(instance.getChannels().getCount(), 0u);
}
TEST_F(TmsIntegrationTest, InputsOutputs)
{
    InstancePtr instance = createDevice();
    auto devices = instance.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);

    auto device1 = devices.getItemAt(0);                    // device 1: No channels
    ASSERT_EQ(device1.getChannels().getCount(), 0u);
    auto device1IoFolder = device1.getInputsOutputsFolder();     // io
    ASSERT_TRUE(device1IoFolder.assigned());
    ASSERT_EQ(device1IoFolder.getItems().getCount(), 0u);


    auto device2 = devices.getItemAt(1);                   // device 2: 4 channels
    ASSERT_EQ(device2.getChannels().getCount(), 4u);

    auto device2IoFolder = device2.getInputsOutputsFolder();    // io: 1 channnel, 2 folders
    ASSERT_TRUE(device2IoFolder.assigned());
    auto ioItems = device2IoFolder.getItems();
    ASSERT_EQ(ioItems.getCount(), 3u);
    ASSERT_EQ(ioItems[0].getName(), "mockch1");                             // io/mockch1
    ASSERT_TRUE(ioItems[0].asPtrOrNull<IFolder>().assigned());
    ASSERT_TRUE(ioItems[0].asPtrOrNull<IChannel>().assigned());

    ASSERT_EQ(ioItems[1].getName(), "mockfolderA");                         // io/mockfolderA: 1 channel
    ASSERT_TRUE(ioItems[1].asPtrOrNull<IFolder>().assigned());
    ASSERT_FALSE(ioItems[1].asPtrOrNull<IChannel>().assigned());

    auto mockFolderAItems = ioItems[1].asPtr<IFolder>().getItems();
    ASSERT_EQ(mockFolderAItems.getCount(), 1u);

    ASSERT_EQ(mockFolderAItems[0].getName(), "mockchA1");                   // io/mockfolderA/mockchA1
    ASSERT_TRUE(mockFolderAItems[0].asPtrOrNull<IFolder>().assigned());
    ASSERT_TRUE(mockFolderAItems[0].asPtrOrNull<IChannel>().assigned());


    ASSERT_EQ(ioItems[2].getName(), "mockfolderB");                         // io/mockFolderB: 2 channels
    ASSERT_TRUE(ioItems[2].asPtrOrNull<IFolder>().assigned());
    ASSERT_FALSE(ioItems[2].asPtrOrNull<IChannel>().assigned());

    auto mockFolderBItems = ioItems[2].asPtr<IFolder>().getItems();
    ASSERT_EQ(mockFolderBItems.getCount(), 2u);

    ASSERT_EQ(mockFolderBItems[0].getName(), "mockchB1");                   // io/mockfolderB/mockchB1
    ASSERT_TRUE(mockFolderBItems[0].asPtrOrNull<IFolder>().assigned());
    ASSERT_TRUE(mockFolderBItems[0].asPtrOrNull<IChannel>().assigned());

    ASSERT_EQ(mockFolderBItems[1].getName(), "mockchB2");                   // io/mockfolderB/mockchB1
    ASSERT_TRUE(mockFolderBItems[1].asPtrOrNull<IFolder>().assigned());
    ASSERT_TRUE(mockFolderBItems[1].asPtrOrNull<IChannel>().assigned());
}

TEST_F(TmsIntegrationTest, CustomComponents)
{
    InstancePtr instance = createDevice();
    auto devices = instance.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);

    auto device2 = devices.getItemAt(1u);
    ASSERT_EQ(device2.getCustomComponents().getCount(), 2u);

    FolderPtr componentA = device2.getCustomComponents()[0];
    ASSERT_EQ(componentA.getItems().getCount(), 1u);
}

TEST_F(TmsIntegrationTest, GetDomainSignal)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    DevicePtr clientDevice;
    {
        TmsClient tmsClient(device.getContext(), nullptr, OPC_URL); // The device should work after we delete the builder
        clientDevice = tmsClient.connect();
    }

    ListPtr<ISignal> signals = clientDevice.getSignals(search::Recursive(search::Visible()));

    auto byteStepSignal = *std::find_if(
        signals.begin(), signals.end(), [](const SignalPtr& signal) { return signal.getDescriptor().getName() == "ByteStep"; });

    SignalPtr domainSignal;
    ASSERT_NO_THROW(domainSignal = byteStepSignal.getDomainSignal());
    ASSERT_TRUE(domainSignal.assigned());
}

TEST_F(TmsIntegrationTest, GetAvailableFunctionBlockTypes)
{
    InstancePtr device = createDevice();
    TmsServer tmsServer(device);
    tmsServer.start();

    auto serverFbTypes = device.getAvailableFunctionBlockTypes();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    auto clientDevice = tmsClient.connect();

    const auto clientFbTypes = clientDevice.getAvailableFunctionBlockTypes();

    ASSERT_EQ(serverFbTypes.getCount(), 1u);
    ASSERT_EQ(serverFbTypes.getCount(), clientFbTypes.getCount());
    ASSERT_TRUE(TestComparators::FunctionBlockTypeEquals(serverFbTypes.get("mock_fb_uid"), clientFbTypes.get("mock_fb_uid")));
}

TEST_F(TmsIntegrationTest, AddFunctionBlock)
{
    InstancePtr device = createDevice();
    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    auto clientDevice = tmsClient.connect();
    ASSERT_EQ("mock_fb_uid_1", clientDevice.getFunctionBlocks()[0].getLocalId());

    auto fb1 = clientDevice.addFunctionBlock("mock_fb_uid");
    ASSERT_TRUE(fb1.assigned());
    ASSERT_EQ("mock_fb_uid_2", fb1.getLocalId());

    auto fb2 = clientDevice.addFunctionBlock("mock_fb_uid");
    ASSERT_TRUE(fb2.assigned());
    ASSERT_EQ("mock_fb_uid_3", fb2.getLocalId());

    ASSERT_EQ(3u, clientDevice.getFunctionBlocks().getCount());
}

TEST_F(TmsIntegrationTest, AddFunctionBlockWitchConfig)
{
    InstancePtr device = createDevice();
    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    auto clientDevice = tmsClient.connect();

    const auto clientFbTypes = clientDevice.getAvailableFunctionBlockTypes();
    ASSERT_TRUE(clientFbTypes.hasKey("mock_fb_uid"));

    auto config = clientFbTypes.get("mock_fb_uid").createDefaultConfig();
    config.setPropertyValue("TestConfigString", "Hello Property!");

    auto fb = clientDevice.addFunctionBlock("mock_fb_uid", config);

    ASSERT_EQ(2u, clientDevice.getFunctionBlocks().getCount());
    ASSERT_EQ(fb.getPropertyValue("TestConfigInt"), 0);
    ASSERT_EQ(fb.getPropertyValue("TestConfigString"), "Hello Property!");
}


TEST_F(TmsIntegrationTest, RemoveFunctionBlock)
{
    InstancePtr device = createDevice();
    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    auto clientDevice = tmsClient.connect();

    auto fb1 = clientDevice.addFunctionBlock("mock_fb_uid");
    auto fb2 = clientDevice.addFunctionBlock("mock_fb_uid");
    ASSERT_EQ(3u, clientDevice.getFunctionBlocks().getCount());

    clientDevice.removeFunctionBlock(fb1);
    ASSERT_EQ(2u, clientDevice.getFunctionBlocks().getCount());

    clientDevice.removeFunctionBlock(fb2);
    ASSERT_EQ(1u, clientDevice.getFunctionBlocks().getCount());
}

TEST_F(TmsIntegrationTest, InputPortConnect)
{
    InstancePtr device = createDevice();
    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    auto clientDevice = tmsClient.connect();

    auto inputPort = clientDevice.getChannelsRecursive().getItemAt(0).getInputPorts().getItemAt(0);
    auto signal1 = clientDevice.getSignalsRecursive().getItemAt(0);
    auto signal2 = clientDevice.getSignalsRecursive().getItemAt(1);

    SignalPtr portSignal = inputPort.getSignal();
    ASSERT_FALSE(portSignal.assigned());

    ASSERT_NO_THROW(inputPort.disconnect());

    inputPort.connect(signal1);
    portSignal = inputPort.getSignal();
    ASSERT_TRUE(portSignal.assigned());
    ASSERT_EQ(portSignal, signal1);

    inputPort.connect(signal2);
    portSignal = inputPort.getSignal();
    ASSERT_TRUE(portSignal.assigned());
    ASSERT_EQ(portSignal, signal2);

    inputPort.disconnect();
    portSignal = inputPort.getSignal();
    ASSERT_FALSE(portSignal.assigned());
}

TEST_F(TmsIntegrationTest, InputPortMultipleServers)
{
    auto StartServerDevice = [&](const InstancePtr& device, uint16_t port)
    {
        auto tmsServer = std::make_shared<TmsServer>(device);
        tmsServer->setOpcUaPort(port);
        tmsServer->start();
        return tmsServer;
    };

    InstancePtr device1 = createDevice();
    InstancePtr device2 = createDevice("localInstance2");
    auto server1 = StartServerDevice(device1, 4001);
    auto server2 = StartServerDevice(device2, 4002);

    InstancePtr instance = Instance();
    auto clientDevice1 = instance.addDevice("daq.opcua://127.0.0.1:4001");
    auto clientDevice2 = instance.addDevice("daq.opcua://127.0.0.1:4002");

    auto inputPort1 = clientDevice1.getChannelsRecursive().getItemAt(0).getInputPorts().getItemAt(0);
    auto inputPort2 = clientDevice2.getChannelsRecursive().getItemAt(0).getInputPorts().getItemAt(0);
    auto signal1 = clientDevice1.getSignalsRecursive().getItemAt(0);
    auto signal2 = clientDevice2.getSignalsRecursive().getItemAt(0);
    SignalPtr portSignal;

    ASSERT_NO_THROW(inputPort1.connect(signal1));
    portSignal = inputPort1.getSignal();
    ASSERT_EQ(portSignal, signal1);

    ASSERT_NO_THROW(inputPort2.connect(signal2));
    portSignal = inputPort2.getSignal();
    ASSERT_EQ(portSignal, signal2);

    ASSERT_THROW(inputPort1.connect(signal2), NotFoundException);
    ASSERT_THROW(inputPort2.connect(signal1), NotFoundException);
}

TEST_F(TmsIntegrationTest, BeginEndUpdateDevice)
{
    InstancePtr device = createDevice();
    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL);
    auto clientDevice = tmsClient.connect();

    ASSERT_NO_THROW(clientDevice.beginUpdate());
    ASSERT_NO_THROW(clientDevice.endUpdate());
}
