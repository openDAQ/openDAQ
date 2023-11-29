#include <opendaq/instance_factory.h>
#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opcuatms_client/tms_client.h>
#include <opcuatms_server/tms_server.h>
#include <opendaq/search_filter_factory.h>
#include <chrono>
#include <thread>

using namespace daq;
using namespace daq::opcua;
using namespace std::chrono_literals;

class TmsIntegrationTest : public testing::Test
{
public:
    const std::string OPC_URL = "opc.tcp://localhost/";

    InstancePtr createDevice()
    {
        const auto moduleManager = ModuleManager("[[none]]");
        auto logger = Logger();
        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        auto instance = InstanceCustom(context, "localInstance");
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

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr);
    DevicePtr clientDevice;
    ASSERT_NO_THROW(clientDevice = tmsClient.connect());
    ASSERT_TRUE(clientDevice.assigned());
}

TEST_F(TmsIntegrationTest, Devices)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr);
    DevicePtr clientDevice = tmsClient.connect();
    auto devices = clientDevice.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);
}

TEST_F(TmsIntegrationTest, DeviceInfo)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr);
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

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr);
    DevicePtr clientDevice = tmsClient.connect();

    auto functionBlocks = clientDevice.getFunctionBlocks();
    ASSERT_EQ(functionBlocks.getCount(), 1u);
}

TEST_F(TmsIntegrationTest, FunctionBlockType)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr);
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

    TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr);
    DevicePtr clientDevice = tmsClient.connect();

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientDevice.getSignals(search::Recursive(search::Visible())));
    ASSERT_EQ(signals.getCount(), device.getSignals(search::Recursive(search::Visible())).getCount());

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
    ASSERT_EQ(device1IoFolder.getItems().getCount(), 0);


    auto device2 = devices.getItemAt(1);                   // device 2: 4 channels
    ASSERT_EQ(device2.getChannels().getCount(), 4u);

    auto device2IoFolder = device2.getInputsOutputsFolder();    // io: 1 channnel, 2 folders
    ASSERT_TRUE(device2IoFolder.assigned());
    auto ioItems = device2IoFolder.getItems();
    ASSERT_EQ(ioItems.getCount(), 3);
    ASSERT_EQ(ioItems[0].getName(), "mockch1");                             // io/mockch1
    ASSERT_TRUE(ioItems[0].asPtrOrNull<IFolder>().assigned());
    ASSERT_TRUE(ioItems[0].asPtrOrNull<IChannel>().assigned());

    ASSERT_EQ(ioItems[1].getName(), "mockfolderA");                         // io/mockfolderA: 1 channel
    ASSERT_TRUE(ioItems[1].asPtrOrNull<IFolder>().assigned());
    ASSERT_FALSE(ioItems[1].asPtrOrNull<IChannel>().assigned());

    auto mockFolderAItems = ioItems[1].asPtr<IFolder>().getItems();
    ASSERT_EQ(mockFolderAItems.getCount(), 1);

    ASSERT_EQ(mockFolderAItems[0].getName(), "mockchA1");                   // io/mockfolderA/mockchA1
    ASSERT_TRUE(mockFolderAItems[0].asPtrOrNull<IFolder>().assigned());
    ASSERT_TRUE(mockFolderAItems[0].asPtrOrNull<IChannel>().assigned());


    ASSERT_EQ(ioItems[2].getName(), "mockfolderB");                         // io/mockFolderB: 2 channels
    ASSERT_TRUE(ioItems[2].asPtrOrNull<IFolder>().assigned());
    ASSERT_FALSE(ioItems[2].asPtrOrNull<IChannel>().assigned());

    auto mockFolderBItems = ioItems[2].asPtr<IFolder>().getItems();
    ASSERT_EQ(mockFolderBItems.getCount(), 2);

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

    auto device2 = devices.getItemAt(1);
    ASSERT_EQ(device2.getCustomComponents().getCount(), 2u);

    FolderPtr componentA = device2.getCustomComponents()[0];
    ASSERT_EQ(componentA.getItems().getCount(), 1);
}

TEST_F(TmsIntegrationTest, GetDomainSignal)
{
    InstancePtr device = createDevice();

    TmsServer tmsServer(device);
    tmsServer.start();

    DevicePtr clientDevice;
    {
        TmsClient tmsClient(device.getContext(), nullptr, OPC_URL, nullptr); // The device should work after we delete the builder
        clientDevice = tmsClient.connect();
    }

    ListPtr<ISignal> signals = clientDevice.getSignals(search::Recursive(search::Visible()));

    auto byteStepSignal = *std::find_if(
        signals.begin(), signals.end(), [](const SignalPtr& signal) { return signal.getDescriptor().getName() == "ByteStep"; });

    SignalPtr domainSignal;
    ASSERT_NO_THROW(domainSignal = byteStepSignal.getDomainSignal());
    ASSERT_TRUE(domainSignal.assigned());
}
