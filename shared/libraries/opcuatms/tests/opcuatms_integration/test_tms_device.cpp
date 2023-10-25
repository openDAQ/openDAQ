#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/unit_factory.h>
#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/mock_physical_device.h>
#include <opendaq/mock/default_mocks_factory.h>
#include <opendaq/folder_config_ptr.h>
#include "opcuatms/exceptions.h"
#include <opcuatms_server/objects/tms_server_device.h>
#include <opcuatms_client/objects/tms_client_device_factory.h>
#include "tms_object_integration_test.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

class TmsDeviceTest : public TmsObjectIntegrationTest
{
public:
    InstancePtr createDevice()
    {
        const auto moduleManager = ModuleManager("[[none]]");
        auto context = Context(nullptr, Logger(), nullptr, moduleManager);
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
};

TEST_F(TmsDeviceTest, CreateClientDevice)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsPropertyObject.registerOpcUaNode();
    auto ctx = NullContext();
    ASSERT_NO_THROW(TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr));
}

TEST_F(TmsDeviceTest, SubDevices)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    ASSERT_EQ(clientDevice.getDevices().getCount(), 2u);
}

TEST_F(TmsDeviceTest, FunctionBlocks)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    ListPtr<IFunctionBlock> functionBlocks;
    ASSERT_NO_THROW(functionBlocks = clientDevice.getFunctionBlocks());
    ASSERT_EQ(functionBlocks.getCount(), 1u);

    auto type = functionBlocks[0].getFunctionBlockType();
    ASSERT_EQ(type.getId(), "mock_fb_uid");
}

TEST_F(TmsDeviceTest, GetSignals)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientDevice.getSignals());
    ASSERT_EQ(signals.getCount(), 0u);

    auto devices = clientDevice.getDevices();
    for (auto subDevice : devices)
    {
        auto name = subDevice.getName();
        ASSERT_NO_THROW(signals = subDevice.getSignals());
        if (name == "mockdev")
            ASSERT_EQ(signals.getCount(), 1u);
        else
            ASSERT_EQ(signals.getCount(), 0u);
    }

    ASSERT_NO_THROW(signals = clientDevice.getSignalsRecursive());
    ASSERT_EQ(signals.getCount(), serverDevice.getSignalsRecursive().getCount());
}

TEST_F(TmsDeviceTest, GetChannels)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = clientDevice.getChannels());
    ASSERT_EQ(channels.getCount(), serverDevice.getChannels().getCount());

    ASSERT_NO_THROW(channels = clientDevice.getChannelsRecursive());
    ASSERT_EQ(channels.getCount(), serverDevice.getChannelsRecursive().getCount());
}

TEST_F(TmsDeviceTest, Property)
{
    DevicePtr serverDevice = createDevice();

    const auto sampleRateProp = FloatPropertyBuilder("SampleRate", 100.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    serverDevice.addProperty(sampleRateProp);

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);

    auto serverVisibleProps = serverDevice.getVisibleProperties();
    auto visibleProperties = clientDevice.getVisibleProperties();
    ASSERT_EQ(visibleProperties.getCount(), 3u);
    ASSERT_EQ(visibleProperties[2].getName(), "SampleRate");

    auto properties = clientDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), 3u);
    ASSERT_EQ(properties[2].getName(), "SampleRate");

    ASSERT_TRUE(clientDevice.hasProperty("SampleRate"));
    ASSERT_EQ(clientDevice.getPropertyValue("SampleRate"), 100.0);

    clientDevice.setPropertyValue("SampleRate", 14.0);
    ASSERT_EQ(clientDevice.getPropertyValue("SampleRate"), 14.0);
    ASSERT_EQ(serverDevice.getPropertyValue("SampleRate"), 14.0);

    ASSERT_EQ(clientDevice.getPropertyValue("UserName"), "");
    ASSERT_EQ(clientDevice.getPropertyValue("Location"), "");
}

TEST_F(TmsDeviceTest, DeviceInfo)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();
    
    auto serverSubDevices = serverDevice.getDevices();
    ASSERT_EQ(serverSubDevices.getCount(), 2u);
    auto serverSubDevice = serverSubDevices[1];
    auto serverDeviceInfo = serverSubDevice.getInfo();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);

    auto clientSubDevices = clientDevice.getDevices();
    ASSERT_EQ(clientSubDevices.getCount(), 2u);
    auto clientSubDevice = clientSubDevices[1];
    auto clientDeviceInfo = clientSubDevice.getInfo();

    //TODO: Test connectionString and Location when implemented
    ASSERT_EQ(clientDeviceInfo.getName(), serverDeviceInfo.getName());
    ASSERT_EQ(clientDeviceInfo.getManufacturer(), serverDeviceInfo.getManufacturer());
    ASSERT_EQ(clientDeviceInfo.getManufacturerUri(), serverDeviceInfo.getManufacturerUri());
    ASSERT_EQ(clientDeviceInfo.getModel(), serverDeviceInfo.getModel());
    ASSERT_EQ(clientDeviceInfo.getProductCode(), serverDeviceInfo.getProductCode());
    ASSERT_EQ(clientDeviceInfo.getDeviceRevision(), serverDeviceInfo.getDeviceRevision());
    ASSERT_EQ(clientDeviceInfo.getHardwareRevision(), serverDeviceInfo.getHardwareRevision());
    ASSERT_EQ(clientDeviceInfo.getSoftwareRevision(), serverDeviceInfo.getSoftwareRevision());
    ASSERT_EQ(clientDeviceInfo.getDeviceManual(), serverDeviceInfo.getDeviceManual());
    ASSERT_EQ(clientDeviceInfo.getDeviceClass(), serverDeviceInfo.getDeviceClass());
    ASSERT_EQ(clientDeviceInfo.getSerialNumber(), serverDeviceInfo.getSerialNumber());
    ASSERT_EQ(clientDeviceInfo.getProductInstanceUri(), serverDeviceInfo.getProductInstanceUri());
    ASSERT_EQ(clientDeviceInfo.getMacAddress(), serverDeviceInfo.getMacAddress());
    ASSERT_EQ(clientDeviceInfo.getParentMacAddress(), serverDeviceInfo.getParentMacAddress());
    ASSERT_EQ(clientDeviceInfo.getPlatform(), serverDeviceInfo.getPlatform());
    ASSERT_EQ(clientDeviceInfo.getPosition(), serverDeviceInfo.getPosition());
    ASSERT_EQ(clientDeviceInfo.getSystemType(), serverDeviceInfo.getSystemType());
    ASSERT_EQ(clientDeviceInfo.getSystemUuid(), serverDeviceInfo.getSystemUuid());


    ASSERT_EQ(clientDeviceInfo.getName(), "MockPhysicalDevice");
    ASSERT_EQ(clientDeviceInfo.getManufacturer(), "manufacturer");
    ASSERT_EQ(clientDeviceInfo.getManufacturerUri(), "manufacturer_uri");
    ASSERT_EQ(clientDeviceInfo.getModel(), "model");
    ASSERT_EQ(clientDeviceInfo.getProductCode(), "product_code");
    ASSERT_EQ(clientDeviceInfo.getHardwareRevision(), "hardware_revision");
    ASSERT_EQ(clientDeviceInfo.getSoftwareRevision(), "software_revision");
    ASSERT_EQ(clientDeviceInfo.getDeviceManual(), "device_manual");
    ASSERT_EQ(clientDeviceInfo.getDeviceClass(), "device_class");
    ASSERT_EQ(clientDeviceInfo.getSerialNumber(), "serial_number");
    ASSERT_EQ(clientDeviceInfo.getProductInstanceUri(), "product_instance_uri");
    ASSERT_EQ(clientDeviceInfo.getRevisionCounter(), 123);
    ASSERT_EQ(clientDeviceInfo.getPropertyValue("custom_string"), "custom_string");
    ASSERT_EQ(clientDeviceInfo.getPropertyValue("custom_float"), 1.123);
    ASSERT_EQ(clientDeviceInfo.getPropertyValue("custom_int"), 1);
}

TEST_F(TmsDeviceTest, DeviceDomain)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevices = serverDevice.getDevices();
    ASSERT_EQ(serverSubDevices.getCount(), 2u);
    auto serverSubDevice = serverSubDevices[1];
    auto serverDeviceInfo = serverSubDevice.getInfo();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);

    auto clientSubDevices = clientDevice.getDevices();
    ASSERT_EQ(clientSubDevices.getCount(), 2u);
    auto clientSubDevice = clientSubDevices[1];

    auto deviceDomain = clientSubDevice.getDomain();

    auto resolution = deviceDomain.getTickResolution();
    ASSERT_EQ(resolution.getNumerator(), 123);
    ASSERT_EQ(resolution.getDenominator(), 456);

    auto ticksSinceOrigin = deviceDomain.getTicksSinceOrigin();
    ASSERT_EQ(ticksSinceOrigin, 789);

    auto origin = deviceDomain.getOrigin();
    ASSERT_EQ(origin, "origin");

    auto unit = deviceDomain.getUnit();
    ASSERT_EQ(unit.getId(), 987);
    ASSERT_EQ(unit.getSymbol(), "unit_symbol");
    ASSERT_EQ(unit.getName(), "unit_name");
    ASSERT_EQ(unit.getQuantity(), "unit_quantity");

}

TEST_F(TmsDeviceTest, CustomComponents)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevice = serverDevice.getDevices()[1];
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    auto clientSubDevice = clientDevice.getDevices()[1];

    ASSERT_EQ(serverSubDevice.getItems().getCount(), clientSubDevice.getItems().getCount());
    auto serverComponentA1 = serverSubDevice.getItem("componentA").asPtr<IFolder>().getItems()[0];
    auto clientComponentA1 = clientSubDevice.getItem("componentA").asPtr<IFolder>().getItems()[0];
    ASSERT_EQ(serverComponentA1.getName(), clientComponentA1.getName());

    auto serverComponentB = serverSubDevice.getItem("componentB");
    auto clientComponentB = clientSubDevice.getItem("componentB");
    ASSERT_EQ(serverComponentB.getName(), clientComponentB.getName());
}

TEST_F(TmsDeviceTest, CustomComponentsProperties)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevice = serverDevice.getDevices()[1];
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    auto clientSubDevice = clientDevice.getDevices()[1];

    ASSERT_EQ(serverSubDevice.getItems().getCount(), clientSubDevice.getItems().getCount());
    auto serverComponentA1 = serverSubDevice.getItem("componentA").asPtr<IFolder>().getItems()[0];
    auto clientComponentA1 = clientSubDevice.getItem("componentA").asPtr<IFolder>().getItems()[0];
    ASSERT_EQ(serverComponentA1.getPropertyValue("StringProp"), clientComponentA1.getPropertyValue("StringProp"));

    auto serverComponentB = serverSubDevice.getItem("componentB");
    auto clientComponentB = clientSubDevice.getItem("componentB");
    ASSERT_EQ(serverComponentB.getName(), clientComponentB.getName());
    ASSERT_EQ(clientComponentB.getPropertyValue("IntProp"), clientComponentB.getPropertyValue("IntProp"));
}

TEST_F(TmsDeviceTest, ComponentMethods)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevice = serverDevice.getDevices()[1];
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    auto clientSubDevice = clientDevice.getDevices()[1];

    ASSERT_EQ(serverSubDevice.getName(), clientSubDevice.getName());

    auto tags = serverSubDevice.getTags();
    auto clientTags = clientSubDevice.getTags();

    ASSERT_TRUE(tags.query("phys_device"));
    ASSERT_TRUE(clientTags.query("phys_device"));

    clientDevice.setActive(false);
    ASSERT_EQ(serverDevice.getActive(), clientDevice.getActive());

    clientDevice.setActive(true);
    ASSERT_EQ(serverDevice.getActive(), clientDevice.getActive());
}

TEST_F(TmsDeviceTest, DeviceProcedureProperty)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = serverTmsDevice.registerOpcUaNode();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId, nullptr);
    auto clientSubDevice = clientDevice.getDevices()[1];

    auto procProp = clientSubDevice.getProperty("stop");
    ASSERT_EQ(procProp.getValueType(), ctProc);

    ProcedurePtr proc = clientSubDevice.getPropertyValue("stop");
    ASSERT_NO_THROW(proc());
}

TEST_F(TmsDeviceTest, SignalOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("sig");
    for (int i = 0; i < 100; ++i)
        folder.addItem(Signal(NullContext(), folder, "sig_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "dev", clientContext, nodeId, nullptr);

    const auto serverSignals = serverDevice.getSignals();
    const auto clientSignals = clientDevice.getSignals();

    for (SizeT i = 0; i < serverSignals.getCount(); ++i)
        ASSERT_EQ(serverSignals[i].getName(), clientSignals[i].getName());
}

TEST_F(TmsDeviceTest, DeviceOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("dev");
    for (int i = 0; i < 100; ++i)
        folder.addItem(DefaultDevice(NullContext(), folder, "dev_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "dev", clientContext, nodeId, nullptr);

    const auto serverDevices = serverDevice.getDevices();
    const auto clientDevices = clientDevice.getDevices();

    for (SizeT i = 0; i < serverDevices.getCount(); ++i)
        ASSERT_EQ(serverDevices[i].getName(), clientDevices[i].getName());
}

TEST_F(TmsDeviceTest, FunctionBlockOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("fb");
    for (int i = 0; i < 100; ++i)
        folder.addItem(DefaultFunctionBlock(NullContext(), folder, "fb_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "dev", clientContext, nodeId, nullptr);

    const auto serverFbs = serverDevice.getFunctionBlocks();
    const auto clientFbs = clientDevice.getFunctionBlocks();

    for (SizeT i = 0; i < serverFbs.getCount(); ++i)
        ASSERT_EQ(serverFbs[i].getName(), clientFbs[i].getName());
}

TEST_F(TmsDeviceTest, IOFolderOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("io");
    for (int i = 0; i < 100; ++i)
    {
        folder.addItem(IoFolder(NullContext(), folder, "cmp_" + std::to_string(i)));
        folder.addItem(DefaultChannel(NullContext(), folder, "ch_" + std::to_string(i)));
    }
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "dev", clientContext, nodeId, nullptr);

    const auto serverIO = serverDevice.getInputsOutputsFolder().getItems();
    const auto clientIO = clientDevice.getInputsOutputsFolder().getItems();

    for (SizeT i = 0; i < serverIO.getCount(); ++i)
        ASSERT_EQ(serverIO[i].getName(), clientIO[i].getName());
}

TEST_F(TmsDeviceTest, CustomComponentOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    auto folder = serverDevice.asPtr<IMockDefaultDevice>();
    for (int i = 0; i < 100; ++i)
        folder->addCustomComponent(Component(NullContext(), folder, "cmp_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), NullContext());
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "dev", clientContext, nodeId, nullptr);

    const auto serverCmps = serverDevice.getCustomComponents();
    const auto clientCmps = clientDevice.getCustomComponents();

    for (SizeT i = 0; i < serverCmps.getCount(); ++i)
        ASSERT_EQ(serverCmps[i].getName(), clientCmps[i].getName());
}
