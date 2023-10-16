#include <opcuatms/exceptions.h>
#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include "testutils/memcheck_listener.h"

using DeviceModulesTest = testing::Test;

// MAC CI issue
#if !defined(SKIP_TEST_MAC_CI)
    #if defined(__clang__) && !defined(__RESHARPER__)
        #define SKIP_TEST_MAC_CI return
    #else
        #define SKIP_TEST_MAC_CI
    #endif   
#endif
using namespace daq;

static InstancePtr CreateServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto context = Context(scheduler, logger, typeManager, moduleManager);

    auto instance = InstanceCustom(context, "local");

    const auto averager = instance.addFunctionBlock("ref_fb_module_averager");
    const auto refDevice = instance.addDevice("daqref://device1");
    averager.getInputPorts()[0].connect(refDevice.getSignalsRecursive()[0]);

    auto sigs = instance.getSignalsRecursive();
    auto avgSigs = averager.getSignalsRecursive();
    auto devSigs = refDevice.getSignalsRecursive();

    instance.addServer("openDAQ OpcUa", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();

    auto config = instance.getAvailableDeviceTypes().get("daq.opcua").createDefaultConfig();
    config.setPropertyValue("StreamingConnectionHeuristic", 3); // 3 - not connected
    auto refDevice = instance.addDevice("daq.opcua://127.0.0.1", config);
    return instance;
}

TEST_F(DeviceModulesTest, ConnectAndDisconnect)  
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
}

TEST_F(DeviceModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    
    auto signals = client.getSignalsRecursive();
    ASSERT_EQ(signals.getCount(), 10u);
    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto fbs = devices[0].getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    auto channels = client.getChannelsRecursive();
    ASSERT_EQ(channels.getCount(), 2u);
}

TEST_F(DeviceModulesTest, GetSetDeviceProperties)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    
    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto serverRefDevice = server.getDevices()[0];

    PropertyPtr propInfo;
    ASSERT_NO_THROW(propInfo = refDevice.getProperty("NumberOfChannels"));
    ASSERT_EQ(propInfo.getValueType(), CoreType::ctInt);
    
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 2);
    refDevice.setPropertyValue("NumberOfChannels", 3);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 3);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 3);

    refDevice.setPropertyValue("NumberOfChannels", 1);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 1);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 1);

    refDevice.setPropertyValue("GlobalSampleRate", 2000);
    ASSERT_EQ(refDevice.getPropertyValue("GlobalSampleRate"), 2000);
    ASSERT_EQ(serverRefDevice.getPropertyValue("GlobalSampleRate"), 2000);

    ASSERT_ANY_THROW(refDevice.setPropertyValue("InvalidProp", 100));

    auto properties = refDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), 5u);
}

TEST_F(DeviceModulesTest, DeviceInfoAndDomain)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto serverRefDeviceInfo = server.getDevices()[0].getInfo();
    auto serverRefDeviceDomain = server.getDevices()[0].getDomain();

    auto info = refDevice.getInfo();
    auto domain = refDevice.getDomain();

    ASSERT_EQ(info.getName(), serverRefDeviceInfo.getName());
    ASSERT_EQ(info.getModel(), serverRefDeviceInfo.getModel());
    ASSERT_EQ(info.getSerialNumber(), serverRefDeviceInfo.getSerialNumber());

    ASSERT_EQ(domain.getTickResolution(), serverRefDeviceDomain.getTickResolution());
    ASSERT_EQ(domain.getOrigin(), serverRefDeviceDomain.getOrigin());
    ASSERT_NO_THROW(domain.getUnit());
    ASSERT_NO_THROW(domain.getTicksSinceOrigin());
}

TEST_F(DeviceModulesTest, DeviceDynamicFeatures)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto daqDevice = client.getDevices()[0];

    ASSERT_EQ(daqDevice.getAvailableDevices().getCount(), 0u);
    ASSERT_EQ(daqDevice.getAvailableFunctionBlockTypes().getCount(), 0u);
    ASSERT_THROW(daqDevice.addDevice("daqref://device0"),
                 opcua::OpcUaClientCallNotAvailableException);  // Are these the correct errors to return?

    auto refDevice = daqDevice.getDevices()[0];
    ASSERT_THROW(daqDevice.removeDevice(refDevice), opcua::OpcUaClientCallNotAvailableException);

    auto refFb = daqDevice.getFunctionBlocks()[0];
    ASSERT_THROW(daqDevice.addFunctionBlock("test_fb"), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(daqDevice.removeFunctionBlock(refFb), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(DeviceModulesTest, DISABLED_Signal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signal = client.getSignalsRecursive()[0];
    auto serverSignal = server.getSignalsRecursive()[0];

    ASSERT_GT(client.getDevices()[0].getSignalsRecursive().getCount(), 0u);
    ASSERT_GT(client.getDevices()[0].getDevices()[0].getSignalsRecursive().getCount(), 0u);

    ASSERT_EQ(signal.getLocalId(), serverSignal.getLocalId());
    ASSERT_EQ(signal.getActive(), serverSignal.getActive());
    signal.setActive(false);
    ASSERT_EQ(signal.getActive(), serverSignal.getActive());
    ASSERT_EQ(signal.getDescriptor().getName().toStdString(), serverSignal.getDescriptor().getName().toStdString());

    // As we have this method available... Should we form connections on the client side?
    // Those connections should not forward packets.
    ASSERT_EQ(signal.getConnections(), 0u);

    ASSERT_THROW(signal.getDomainSignal(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(signal.getRelatedSignals(), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(DeviceModulesTest, SignalConfig_Server)
{
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = server.getSignalsRecursive()[0].asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignalsRecursive();
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();
    
    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_F(DeviceModulesTest, DISABLED_SignalConfig_Client)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignalsRecursive();
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto descCopy = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).setName("test123").build();

    // This currently fails. Should throw OpcUaClientCallNotAvailableException if we cannot fix it.
    // causes UA_STATUSCODE_BADWRITENOTSUPPORTED: The server does not support writing the combination of value
    ASSERT_NO_THROW(clientSignal.setDescriptor(descCopy)); 
    ASSERT_TRUE(descCopy.isFrozen());
    ASSERT_EQ(descCopy.getName(), clientSignal.getDescriptor().getName());

    ASSERT_THROW(clientSignal.setDomainSignal(clientSignals[3]), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.setRelatedSignals(List<ISignal>()), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.addRelatedSignal(Signal(nullptr, nullptr, "sig")), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.removeRelatedSignal(Signal(nullptr, nullptr, "sig")), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.clearRelatedSignals(), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(DeviceModulesTest, DISABLED_SignalLocalConnections)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignalsRecursive()[0].asPtr<ISignalConfig>();
    auto reader = PacketReader(signal);
    ASSERT_EQ(signal.getConnections().getCount(), 1u);

    reader.release();
    ASSERT_EQ(signal.getConnections().getCount(), 0u);
}

TEST_F(DeviceModulesTest, SignalDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signalDescriptor = client.getSignalsRecursive()[0].getDescriptor();
    auto serverSignalDescriptor = server.getSignalsRecursive()[0].getDescriptor();

    ASSERT_EQ(signalDescriptor.getName(), serverSignalDescriptor.getName());
    ASSERT_EQ(signalDescriptor.getMetadata(), serverSignalDescriptor.getMetadata());
}

TEST_F(DeviceModulesTest, ChannelProps)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto dev = client.getDevices()[0].getDevices()[0];
    auto customRangeValue = dev.getChannels()[0].getPropertyValue("CustomRange").asPtr<IStruct>();

    ASSERT_EQ(customRangeValue.get("lowValue"), -10.0);
    ASSERT_EQ(customRangeValue.get("highValue"), 10.0);
}


TEST_F(DeviceModulesTest, DataDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignalsRecursive()[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignalsRecursive()[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignalsRecursive()[2].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignalsRecursive()[2].getDescriptor();

    ASSERT_EQ(dataDescriptor.getName(), serverDataDescriptor.getName());
    ASSERT_EQ(dataDescriptor.getDimensions().getCount(), serverDataDescriptor.getDimensions().getCount());

    ASSERT_EQ(dataDescriptor.getSampleType(), serverDataDescriptor.getSampleType());
    ASSERT_EQ(dataDescriptor.getUnit().getSymbol(), serverDataDescriptor.getUnit().getSymbol());
    ASSERT_EQ(dataDescriptor.getValueRange(), serverDataDescriptor.getValueRange());
    ASSERT_EQ(dataDescriptor.getRule().getType(), serverDataDescriptor.getRule().getType());

    ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
    ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
    ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());

    auto refChannel = client.getChannelsRecursive()[0];
    refChannel.setPropertyValue("ClientSideScaling", true);

    dataDescriptor = client.getChannelsRecursive()[0].getSignalsRecursive()[0].getDescriptor();
    serverDataDescriptor = server.getChannelsRecursive()[0].getSignalsRecursive()[0].getDescriptor();
    ASSERT_EQ(dataDescriptor.getPostScaling().getParameters(), dataDescriptor.getPostScaling().getParameters());
}

TEST_F(DeviceModulesTest, DISABLED_FunctionBlock)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    auto fbType = fb.getFunctionBlockType();
    auto serverFbType = serverFb.getFunctionBlockType();

    ASSERT_EQ(fbType.getId(), serverFbType.getId());
    ASSERT_EQ(fbType.getDescription(), serverFbType.getDescription());
    ASSERT_EQ(fbType.getName(), serverFbType.getName());

    ASSERT_EQ(fb.getInputPorts().getCount(), serverFb.getInputPorts().getCount());
    ASSERT_EQ(fb.getSignalsRecursive().getCount(), serverFb.getSignalsRecursive().getCount());

    auto fbSignal = fb.getSignalsRecursive()[0];
    auto serverFbSignal = serverFb.getSignalsRecursive()[0];

    ASSERT_EQ(fbSignal.getLocalId(), serverFbSignal.getLocalId());
    ASSERT_EQ(fbSignal.getActive(), serverFbSignal.getActive());
    ASSERT_EQ(fbSignal.getDescriptor().getName(), serverFbSignal.getDescriptor().getName());
    ASSERT_EQ(fbSignal.getDescriptor().getSampleType(), serverFbSignal.getDescriptor().getSampleType());

    ASSERT_NO_THROW(fb.getStatusSignal());

    auto notifications = fb.asPtr<IInputPortNotifications>();
    ASSERT_THROW(notifications.acceptsSignal(nullptr, nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(notifications.connected(nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(notifications.disconnected(nullptr), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(DeviceModulesTest, FunctionBlockProperties)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    fb.setPropertyValue("BlockSize", 20);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));
    fb.setPropertyValue("DomainSignalType", 2);
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));

    ASSERT_ANY_THROW(fb.setPropertyValue("DomainSignalType" , 1000));
}

TEST_F(DeviceModulesTest, DISABLED_InputPort)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto port = client.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto serverPort = server.getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_EQ(port.getLocalId(), serverPort.getLocalId());
    ASSERT_EQ(port.getRequiresSignal(), serverPort.getRequiresSignal());

    ASSERT_THROW(port.acceptsSignal(nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.connect(nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.disconnect(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.getSignal(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.getConnection(), opcua::OpcUaClientCallNotAvailableException);

    auto portConfig = port.asPtr<IInputPortConfig>();
    ASSERT_THROW(portConfig.getCustomData(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(portConfig.notifyPacketEnqueued(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(portConfig.setNotificationMethod(PacketReadyNotification::SameThread), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(portConfig.setCustomData(nullptr), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(DeviceModulesTest, DISABLED_PublicProp)
{
    auto server = Instance();
    const auto refDevice = server.addDevice("daqref://device1");
    refDevice.getSignalsRecursive()[0].setPublic(false);
    auto id = refDevice.getSignalsRecursive()[0].getLocalId();
    server.addServer("openDAQ OpcUa", nullptr);
    auto client = CreateClientInstance();

    ASSERT_NE(client.getDevices()[0].getDevices()[0].getSignalsRecursive()[0].getLocalId(), id);
}

TEST_F(DeviceModulesTest, ProcedureProp)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto ch = client.getDevices()[0].getDevices()[0].getChannels()[0];
    ch.setPropertyValue("Waveform", 3);
    ProcedurePtr proc = ch.getPropertyValue("ResetCounter");
    ASSERT_NO_THROW(proc());
}

////////
// Tests defining future requirements
////////

TEST_F(DeviceModulesTest, DISABLED_ReferenceMethods)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    
    auto signals = client.getDevices()[0].getDevices()[0].getSignalsRecursive();
    auto connectedSignal = signals[0];
    auto domainSignal = signals[1];

    auto inputPort = client.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_EQ(inputPort.getSignal(), connectedSignal);
    ASSERT_EQ(inputPort.getConnection(), connectedSignal.getConnections()[0]);
    ASSERT_EQ(connectedSignal.getDomainSignal(), domainSignal);

    // TODO: We have no related signals example. Should be added when we do.
}

TEST_F(DeviceModulesTest, DISABLED_DynamicSignalConfig)
{
    auto server = CreateServerInstance();
    auto serverDevice = server.addDevice("daqref://device0");
    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getDevices()[0].getSignalsRecursive();
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();
    
    clientSignal.setDomainSignal(clientSignals[1]);
    ASSERT_EQ(serverDevice.getSignalsRecursive()[0].getDomainSignal().getLocalId(), clientSignal.getDomainSignal().getLocalId());
}

TEST_F(DeviceModulesTest, FunctionBlocksOnClient)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto context = Context(scheduler, logger, typeManager, moduleManager);

    auto instance = InstanceCustom(context, "local");
    
    instance.setRootDevice("daqref://device1");

    const auto averager = instance.addFunctionBlock("ref_fb_module_averager");
    instance.addServer("openDAQ OpcUa", nullptr);
    auto client = CreateClientInstance();

    ASSERT_GT(client.getDevices()[0].getFunctionBlocks().getCount(), (SizeT) 0);
}

// TODO: Add all examples of dynamic changes
// TODO: Add examples of all possible property object changes once rework is done
// TODO: Add examples of device topology, as well as signal/channel node trees in devices/fbs
// TODO: Add examples of streaming tests once streaming is available
