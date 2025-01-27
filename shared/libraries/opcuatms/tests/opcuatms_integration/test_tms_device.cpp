#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/unit_factory.h>
#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/mock_physical_device.h>
#include <opendaq/mock/default_mocks_factory.h>
#include <opendaq/folder_config_ptr.h>
#include <opcuatms/exceptions.h>
#include <opcuatms_server/objects/tms_server_device.h>
#include <opcuatms_client/objects/tms_client_device_factory.h>
#include <opendaq/search_filter_factory.h>
#include "tms_object_integration_test.h"
#include <opendaq/device_info_internal_ptr.h>

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
        auto context = Context(nullptr, Logger(), nullptr, moduleManager, nullptr);
        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        auto instance = InstanceCustom(context, "localInstance");
        instance.addDevice("daqmock://client_device");
        const auto device = instance.addDevice("daqmock://phys_device");
        const auto infoInternal = device.getInfo().asPtr<IDeviceInfoInternal>();
        infoInternal.addServerCapability(ServerCapability("protocol_1", "protocol 1", ProtocolType::Streaming));
        infoInternal.addServerCapability(ServerCapability("protocol_2", "protocol 2", ProtocolType::Configuration));

        instance.addFunctionBlock("mock_fb_uid");

        return instance;
    }
};

TEST_F(TmsDeviceTest, CreateClientDevice)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();
    auto ctx = NullContext();
    ASSERT_NO_THROW(TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId));
}

TEST_F(TmsDeviceTest, SubDevices)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    ASSERT_EQ(clientDevice.getDevices().getCount(), 2u);
}

TEST_F(TmsDeviceTest, FunctionBlocks)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    ListPtr<IFunctionBlock> functionBlocks;
    ASSERT_NO_THROW(functionBlocks = clientDevice.getFunctionBlocks());
    ASSERT_EQ(functionBlocks.getCount(), 1u);

    auto type = functionBlocks[0].getFunctionBlockType();
    ASSERT_EQ(type.getId(), "mock_fb_uid");
}

TEST_F(TmsDeviceTest, GetSignals)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientDevice.getSignals());
    ASSERT_EQ(signals.getCount(), 0u);

    auto devices = clientDevice.getDevices();
    for (auto subDevice : devices)
    {
        auto name = subDevice.getName();
        ASSERT_NO_THROW(signals = subDevice.getSignals());
        if (name == "MockPhysicalDevice")
            ASSERT_EQ(signals.getCount(), 1u);
        else
            ASSERT_EQ(signals.getCount(), 0u);
    }

    ASSERT_NO_THROW(signals = clientDevice.getSignals(search::Recursive(search::Visible())));
    // one private signal in MockFunctionBlockImpl. and one in MockPhysicalDeviceImpl
    ASSERT_EQ(signals.getCount(), serverDevice.getSignals(search::Recursive(search::Visible())).getCount() - 2);
}

TEST_F(TmsDeviceTest, GetChannels)
{
    DevicePtr serverDevice = createDevice();

    auto tmsPropertyObject = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = clientDevice.getChannels());
    ASSERT_EQ(channels.getCount(), serverDevice.getChannels().getCount());

    ASSERT_NO_THROW(channels = clientDevice.getChannels(search::Recursive(search::Visible())));
    ASSERT_EQ(channels.getCount(), serverDevice.getChannels(search::Recursive(search::Visible())).getCount());
}

// TODO: Enable once name and description are no longer props
TEST_F(TmsDeviceTest, DISABLED_Property)
{
    DevicePtr serverDevice = createDevice();

    const auto sampleRateProp = FloatPropertyBuilder("SampleRate", 100.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    serverDevice.addProperty(sampleRateProp);

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto ctx = NullContext();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);

    auto serverVisibleProps = serverDevice.getVisibleProperties();
    auto visibleProperties = clientDevice.getVisibleProperties();
    ASSERT_EQ(visibleProperties.getCount(), 5u);
    ASSERT_EQ(visibleProperties[4].getName(), "SampleRate");

    auto properties = clientDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), 5u);
    ASSERT_EQ(properties[4].getName(), "SampleRate");

    ASSERT_TRUE(clientDevice.hasProperty("SampleRate"));
    ASSERT_EQ(clientDevice.getPropertyValue("SampleRate"), 100.0);

    clientDevice.setPropertyValue("SampleRate", 14.0);
    ASSERT_EQ(clientDevice.getPropertyValue("SampleRate"), 14.0);
    ASSERT_EQ(serverDevice.getPropertyValue("SampleRate"), 14.0);

    ASSERT_EQ(clientDevice.getPropertyValue("userName"), "");
    ASSERT_EQ(clientDevice.getPropertyValue("location"), "");
}

TEST_F(TmsDeviceTest, DeviceInfo)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();
    
    auto serverSubDevices = serverDevice.getDevices();
    ASSERT_EQ(serverSubDevices.getCount(), 2u);
    auto serverSubDevice = serverSubDevices[1];
    auto serverDeviceInfo = serverSubDevice.getInfo();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);

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

TEST_F(TmsDeviceTest, DeviceInfoServerCapabilities)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();
    
    auto serverSubDevices = serverDevice.getDevices();
    ASSERT_EQ(serverSubDevices.getCount(), 2u);
    auto serverSubDevice = serverSubDevices[1];
    auto serverDeviceInfo = serverSubDevice.getInfo();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);

    auto clientSubDevices = clientDevice.getDevices();
    ASSERT_EQ(clientSubDevices.getCount(), 2u);
    auto clientSubDevice = clientSubDevices[1];
    auto clientDeviceInfo = clientSubDevice.getInfo();
    ASSERT_EQ(serverDeviceInfo.getServerCapabilities().getCount(), 2u);
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities().getCount(), 2u);
    auto name = clientDeviceInfo.getServerCapabilities()[1].getProtocolName();
    auto id = clientDeviceInfo.getServerCapabilities()[1].getProtocolId();
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[0].getProtocolId(), serverDeviceInfo.getServerCapabilities()[0].getProtocolId());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[1].getProtocolId(), serverDeviceInfo.getServerCapabilities()[1].getProtocolId());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[0].getProtocolName(), serverDeviceInfo.getServerCapabilities()[0].getProtocolName());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[1].getProtocolName(), serverDeviceInfo.getServerCapabilities()[1].getProtocolName());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[0].getProtocolType(), serverDeviceInfo.getServerCapabilities()[0].getProtocolType());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[1].getProtocolType(), serverDeviceInfo.getServerCapabilities()[1].getProtocolType());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[0].getConnectionType(), serverDeviceInfo.getServerCapabilities()[0].getConnectionType());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[1].getConnectionType(), serverDeviceInfo.getServerCapabilities()[1].getConnectionType());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[0].getCoreEventsEnabled(), serverDeviceInfo.getServerCapabilities()[0].getCoreEventsEnabled());
    ASSERT_EQ(clientDeviceInfo.getServerCapabilities()[1].getCoreEventsEnabled(), serverDeviceInfo.getServerCapabilities()[1].getCoreEventsEnabled());
}

TEST_F(TmsDeviceTest, DeviceGetTicksSinceOrigin)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    auto clientSubDevices = clientDevice.getDevices();
    auto clientSubDevice = clientSubDevices[1];

    auto ticksSinceOrigin = clientSubDevice.getTicksSinceOrigin();
    ASSERT_EQ(ticksSinceOrigin, 789);
}

TEST_F(TmsDeviceTest, DeviceDomain)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevices = serverDevice.getDevices();
    ASSERT_EQ(serverSubDevices.getCount(), 2u);
    auto serverSubDevice = serverSubDevices[1];
    auto serverDeviceInfo = serverSubDevice.getInfo();

    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);

    auto clientSubDevices = clientDevice.getDevices();
    ASSERT_EQ(clientSubDevices.getCount(), 2u);
    auto clientSubDevice = clientSubDevices[1];

    auto deviceDomain = clientSubDevice.getDomain();

    auto resolution = deviceDomain.getTickResolution();
    ASSERT_EQ(resolution.getNumerator(), 123);
    ASSERT_EQ(resolution.getDenominator(), 456);

    auto ticksSinceOrigin = clientSubDevice.getTicksSinceOrigin();
    ASSERT_EQ(ticksSinceOrigin, 789);

    auto origin = deviceDomain.getOrigin();
    ASSERT_EQ(origin, "Origin");

    auto unit = deviceDomain.getUnit();
    ASSERT_EQ(unit.getId(), 987);
    ASSERT_EQ(unit.getSymbol(), "UnitSymbol");
    ASSERT_EQ(unit.getName(), "UnitName");
    ASSERT_EQ(unit.getQuantity(), "UnitQuantity");

}

TEST_F(TmsDeviceTest, CustomComponents)
{
    auto ctx = NullContext();
    DevicePtr serverDevice = createDevice();

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevice = serverDevice.getDevices()[1];
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "Dev", clientContext, nodeId);
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

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevice = serverDevice.getDevices()[1];
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "Dev", clientContext, nodeId);
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

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();

    auto serverSubDevice = serverDevice.getDevices()[1];
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "Dev", clientContext, nodeId);
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

    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = serverTmsDevice.registerOpcUaNode();
    auto clientDevice = TmsClientRootDevice(ctx, nullptr, "Dev", clientContext, nodeId);
    auto clientSubDevice = clientDevice.getDevices()[1];

    auto procProp = clientSubDevice.getProperty("stop");
    ASSERT_EQ(procProp.getValueType(), ctProc);

    ProcedurePtr proc = clientSubDevice.getPropertyValue("stop");
    ASSERT_NO_THROW(proc());
}

TEST_F(TmsDeviceTest, SignalOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("Sig");
    for (int i = 0; i < 100; ++i)
        folder.addItem(Signal(NullContext(), folder, "sig_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "Dev", clientContext, nodeId);

    const auto serverSignals = serverDevice.getSignals();
    const auto clientSignals = clientDevice.getSignals();

    for (SizeT i = 0; i < serverSignals.getCount(); ++i)
        ASSERT_EQ(serverSignals[i].getName(), clientSignals[i].getName());
}

TEST_F(TmsDeviceTest, DeviceOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("Dev");
    for (int i = 0; i < 100; ++i)
        folder.addItem(DefaultDevice(NullContext(), folder, "dev_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "Dev", clientContext, nodeId);

    const auto serverDevices = serverDevice.getDevices();
    const auto clientDevices = clientDevice.getDevices();

    for (SizeT i = 0; i < serverDevices.getCount(); ++i)
        ASSERT_EQ(serverDevices[i].getName(), clientDevices[i].getName());
}

TEST_F(TmsDeviceTest, FunctionBlockOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("FB");
    for (int i = 0; i < 100; ++i)
        folder.addItem(DefaultFunctionBlock(NullContext(), folder, "fb_" + std::to_string(i)));
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "Dev", clientContext, nodeId);

    const auto serverFbs = serverDevice.getFunctionBlocks();
    const auto clientFbs = clientDevice.getFunctionBlocks();

    for (SizeT i = 0; i < serverFbs.getCount(); ++i)
        ASSERT_EQ(serverFbs[i].getName(), clientFbs[i].getName());
}

TEST_F(TmsDeviceTest, IOFolderOrder)
{
    auto serverDevice = DefaultDevice(NullContext(), nullptr, "mock");
    FolderConfigPtr folder = serverDevice.getItem("IO");
    for (int i = 0; i < 100; ++i)
    {
        folder.addItem(IoFolder(NullContext(), folder, "cmp_" + std::to_string(i)));
        folder.addItem(DefaultChannel(NullContext(), folder, "ch_" + std::to_string(i)));
    }
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "Dev", clientContext, nodeId);

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
    
    auto tmsServerDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "Dev", clientContext, nodeId);

    const auto serverCmps = serverDevice.getCustomComponents();
    const auto clientCmps = clientDevice.getCustomComponents();

    for (SizeT i = 0; i < serverCmps.getCount(); ++i)
        ASSERT_EQ(serverCmps[i].getName(), clientCmps[i].getName());
}

TEST_F(TmsDeviceTest, SdkPackageVersion)
{
    auto serverDevice = createDevice();
    auto tmsServerDevice = TmsServerDevice(serverDevice.getRootDevice(), this->getServer(), ctx, serverContext);
    auto nodeId = tmsServerDevice.registerOpcUaNode();
    DevicePtr clientDevice = TmsClientRootDevice(NullContext(), nullptr, "Dev", clientContext, nodeId);

    ASSERT_EQ(clientDevice.getInfo().getSdkVersion(), OPENDAQ_PACKAGE_VERSION);
}

TEST_F(TmsDeviceTest, DeviceInfoChanges)
{
    const auto ctx = NullContext();
    const DevicePtr serverDevice = createDevice();
     
    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    const auto nodeId = serverTmsDevice.registerOpcUaNode();
    const auto serverDeviceInfo = serverDevice.getDevices()[1].getInfo();
     
    const auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    const auto clientSubDevice = clientDevice.getDevices()[1];
    const auto clientDeviceInfo = clientSubDevice.getInfo();

    ASSERT_EQ(serverDeviceInfo.getName(), clientDeviceInfo.getName());
    ASSERT_EQ(serverDeviceInfo.getLocation(), clientDeviceInfo.getLocation());

    clientSubDevice.setName("new_name");
    clientSubDevice.setPropertyValue("location", "new_location");
    
    ASSERT_EQ("new_name", clientDeviceInfo.getName());
    ASSERT_EQ("new_location", clientDeviceInfo.getLocation());

    ASSERT_EQ(serverDeviceInfo.getName(), clientDeviceInfo.getName());
    ASSERT_EQ(serverDeviceInfo.getLocation(), clientDeviceInfo.getLocation());
}

TEST_F(TmsDeviceTest, DeviceInfoChangeableField)
{
    const auto ctx = NullContext();
    const DevicePtr serverDevice = createDevice();
     
    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    const auto nodeId = serverTmsDevice.registerOpcUaNode();
    const auto serverDeviceInfo = serverDevice.getDevices()[1].getInfo();
     
    const auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    const auto clientSubDevice = clientDevice.getDevices()[1];
    const auto clientDeviceInfo = clientSubDevice.getInfo();

    ASSERT_EQ(serverDeviceInfo.getPropertyValue("TestChangeableField"), clientDeviceInfo.getPropertyValue("TestChangeableField"));

    serverDeviceInfo.setPropertyValue("TestChangeableField", "new_value");
    ASSERT_EQ("new_value", serverDeviceInfo.getPropertyValue("TestChangeableField"));
    ASSERT_EQ("new_value", clientDeviceInfo.getPropertyValue("TestChangeableField"));

    clientDeviceInfo.setPropertyValue("TestChangeableField", "new_value_2");
    ASSERT_EQ("new_value_2", serverDeviceInfo.getPropertyValue("TestChangeableField"));
    ASSERT_EQ("new_value_2", clientDeviceInfo.getPropertyValue("TestChangeableField"));
}

TEST_F(TmsDeviceTest, DeviceInfoNotChangeableField)
{
    const auto ctx = NullContext();
    const DevicePtr serverDevice = createDevice();
     
    auto serverTmsDevice = TmsServerDevice(serverDevice, this->getServer(), ctx, serverContext);
    const auto nodeId = serverTmsDevice.registerOpcUaNode();
    const auto serverDeviceInfo = serverDevice.getDevices()[1].getInfo();
     
    const auto clientDevice = TmsClientRootDevice(ctx, nullptr, "dev", clientContext, nodeId);
    const auto clientSubDevice = clientDevice.getDevices()[1];
    const auto clientDeviceInfo = clientSubDevice.getInfo();

    ASSERT_EQ("manufacturer", serverDeviceInfo.getManufacturer());
    ASSERT_EQ("manufacturer", clientDeviceInfo.getManufacturer());
    
    {
        ASSERT_ANY_THROW(serverDeviceInfo.setPropertyValue("manufacturer", "server_manufacturer"));
        ASSERT_EQ("manufacturer", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("manufacturer", clientDeviceInfo.getManufacturer());

        serverDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("manufacturer", "server_manufacturer_2");
        ASSERT_EQ("server_manufacturer_2", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("manufacturer", clientDeviceInfo.getManufacturer());

        serverDeviceInfo.asPtr<IDeviceInfoConfig>(true).setManufacturer("server_manufacturer_3");
        ASSERT_EQ("server_manufacturer_3", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("manufacturer", clientDeviceInfo.getManufacturer());
    }

    {
        ASSERT_ANY_THROW(clientDeviceInfo.setPropertyValue("manufacturer", "client_manufacturer"));
        ASSERT_EQ("server_manufacturer_3", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("manufacturer", clientDeviceInfo.getManufacturer());

        clientDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("manufacturer", "client_manufacturer_2");
        ASSERT_EQ("server_manufacturer_3", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("client_manufacturer_2", clientDeviceInfo.getManufacturer());

        clientDeviceInfo.asPtr<IDeviceInfoConfig>(true).setManufacturer("client_manufacturer_3");
        ASSERT_EQ("server_manufacturer_3", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("client_manufacturer_3", clientDeviceInfo.getManufacturer());
    }
}