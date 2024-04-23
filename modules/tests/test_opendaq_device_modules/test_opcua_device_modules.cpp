#include <opcuatms/exceptions.h>
#include <opendaq/logger_sink_ptr.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <opcuashared/opcuaexception.h>
#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>

using OpcuaDeviceModulesTest = testing::Test;

using namespace daq;

static InstancePtr CreateServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    const auto refDevice = instance.addDevice("daqref://device1");
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);
    statistics.getInputPorts()[0].connect(Signal(context, nullptr, "foo"));

    instance.addServer("openDAQ OpcUa", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance(const InstanceBuilderPtr& builder = InstanceBuilder())
{
    auto instance = builder.build();

    auto refDevice = instance.addDevice("daq.opcua://127.0.0.1");
    return instance;
}


TEST_F(OpcuaDeviceModulesTest, ConnectAndDisconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(OpcuaDeviceModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = CreateServerInstance();
    auto client = Instance();
    ASSERT_NO_THROW(client.addDevice("daq.opcua://[::1]"));
}

TEST_F(OpcuaDeviceModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Any()));
    auto signalsServer = server.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(signals.getCount(), 7u);
    auto signalsVisible = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signalsVisible.getCount(), 4u);
    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto fbs = devices[0].getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    auto channels = client.getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 2u);
}

TEST_F(OpcuaDeviceModulesTest, RemoteGlobalIds)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto clientRootId = client.getGlobalId();
    const auto serverDeviceId = server.getGlobalId();
    const auto clientDeviceId = client.getDevices()[0].getGlobalId();

    const auto serverSignalId = server.getSignalsRecursive()[0].getGlobalId();
    const auto clientSignalId = client.getSignalsRecursive()[0].getGlobalId();

    ASSERT_EQ(clientDeviceId, clientRootId + "/Dev" + serverDeviceId);
    ASSERT_EQ(clientSignalId, clientRootId + "/Dev" + serverSignalId);
}

TEST_F(OpcuaDeviceModulesTest, GetSetDeviceProperties)
{
    SKIP_TEST_MAC_CI;
    auto loggerSink = LastMessageLoggerSink();
    loggerSink.setLevel(LogLevel::Warn);
    auto debugSink = loggerSink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();

    auto sinks = DefaultSinks(nullptr);
    sinks.pushBack(loggerSink);
    auto logger = LoggerWithSinks(sinks);

    auto server = CreateServerInstance();
    auto client = CreateClientInstance(InstanceBuilder().setLogger(logger));

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

    auto oldProperties = refDevice.getAllProperties();

    // reset messages
    debugSink.waitForMessage(0);
    ASSERT_ANY_THROW(refDevice.setPropertyValue("InvalidProp", 100));
    logger.flush();
    ASSERT_TRUE(debugSink.waitForMessage(2000));
    ASSERT_EQ(debugSink.getLastMessage(), "Failed to set value for property \"InvalidProp\" on OpcUA client property object: Property not found");

    auto properties = refDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), oldProperties.getCount());
}

TEST_F(OpcuaDeviceModulesTest, DeviceInfoAndDomain)
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
    ASSERT_NO_THROW(refDevice.getTicksSinceOrigin());
}

TEST_F(OpcuaDeviceModulesTest, DeviceDynamicFeatures)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto daqDevice = client.getDevices()[0];

    ASSERT_EQ(daqDevice.getAvailableDevices().getCount(), 0u);
    ASSERT_EQ(daqDevice.getAvailableFunctionBlockTypes().getCount(), 8u);
    ASSERT_THROW(daqDevice.addDevice("daqref://device0"),
                 opcua::OpcUaClientCallNotAvailableException);  // Are these the correct errors to return?

    auto refDevice = daqDevice.getDevices()[0];
    ASSERT_THROW(daqDevice.removeDevice(refDevice), opcua::OpcUaClientCallNotAvailableException);

    auto refFb = daqDevice.getFunctionBlocks()[0];
    ASSERT_THROW(daqDevice.addFunctionBlock("test_fb"), daq::GeneralErrorException);

    auto scalingFb = daqDevice.addFunctionBlock("ref_fb_module_scaling");
    ASSERT_TRUE(scalingFb.assigned());

    ASSERT_NO_THROW(daqDevice.removeFunctionBlock(refFb));
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_Signal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signal = client.getSignals(search::Recursive(search::Visible()))[0];
    auto serverSignal = server.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_GT(client.getDevices()[0].getSignals(search::Recursive(search::Visible())).getCount(), 0u);
    ASSERT_GT(client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible())).getCount(), 0u);

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

TEST_F(OpcuaDeviceModulesTest, SignalConfig_Server)
{
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = server.getSignals(search::Recursive(search::Visible()))[0].asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();

    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_SignalConfig_Client)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Visible()));
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

TEST_F(OpcuaDeviceModulesTest, DISABLED_SignalLocalConnections)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignals(search::Recursive(search::Visible()))[0].asPtr<ISignalConfig>();
    auto reader = PacketReader(signal);
    ASSERT_EQ(signal.getConnections().getCount(), 1u);

    reader.release();
    ASSERT_EQ(signal.getConnections().getCount(), 0u);
}

TEST_F(OpcuaDeviceModulesTest, SignalDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signalDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    auto serverSignalDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    ASSERT_EQ(signalDescriptor.getName(), serverSignalDescriptor.getName());
    ASSERT_EQ(signalDescriptor.getMetadata(), serverSignalDescriptor.getMetadata());
}

TEST_F(OpcuaDeviceModulesTest, DataDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();

    ASSERT_EQ(dataDescriptor.getName(), serverDataDescriptor.getName());
    ASSERT_EQ(dataDescriptor.getDimensions().getCount(), serverDataDescriptor.getDimensions().getCount());

    ASSERT_EQ(dataDescriptor.getSampleType(), serverDataDescriptor.getSampleType());
    ASSERT_EQ(dataDescriptor.getUnit().getSymbol(), serverDataDescriptor.getUnit().getSymbol());
    ASSERT_EQ(dataDescriptor.getValueRange(), serverDataDescriptor.getValueRange());
    ASSERT_EQ(dataDescriptor.getRule().getType(), serverDataDescriptor.getRule().getType());

    ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
    ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
    ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());

    auto refChannel = client.getChannels(search::Recursive(search::Visible()))[0];
    refChannel.setPropertyValue("ClientSideScaling", true);

    dataDescriptor = client.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    serverDataDescriptor = server.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    ASSERT_EQ(dataDescriptor.getPostScaling().getParameters(), dataDescriptor.getPostScaling().getParameters());
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_FunctionBlock)
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
    ASSERT_EQ(fb.getSignals(search::Recursive(search::Visible())).getCount(), serverFb.getSignals(search::Recursive(search::Visible())).getCount());

    auto fbSignal = fb.getSignals(search::Recursive(search::Visible()))[0];
    auto serverFbSignal = serverFb.getSignals(search::Recursive(search::Visible()))[0];

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

TEST_F(OpcuaDeviceModulesTest, FunctionBlockProperties)
{
    auto loggerSink = LastMessageLoggerSink();
    loggerSink.setLevel(LogLevel::Warn);
    auto debugSink = loggerSink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();

    auto sinks = DefaultSinks(nullptr);
    sinks.pushBack(loggerSink);
    auto logger = LoggerWithSinks(sinks);

    auto server = CreateServerInstance();
    auto client = CreateClientInstance(InstanceBuilder().setLogger(logger));

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    fb.setPropertyValue("BlockSize", 20);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));

    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));
    fb.setPropertyValue("DomainSignalType", 2);
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));

    // reset messages
    debugSink.waitForMessage(0);
    ASSERT_NO_THROW(fb.setPropertyValue("DomainSignalType" , 1000));
    logger.flush();
    ASSERT_TRUE(debugSink.waitForMessage(2000));
    ASSERT_EQ(debugSink.getLastMessage(), "Failed to set value for property \"DomainSignalType\" on OpcUA client property object: Writing property value");
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_InputPort)
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

TEST_F(OpcuaDeviceModulesTest, DISABLED_PublicProp)
{
    auto server = Instance();
    const auto refDevice = server.addDevice("daqref://device1");
    refDevice.getSignals(search::Recursive(search::Visible()))[0].setPublic(false);
    auto id = refDevice.getSignals(search::Recursive(search::Visible()))[0].getLocalId();
    server.addServer("openDAQ OpcUa", nullptr);
    auto client = CreateClientInstance();

    ASSERT_NE(client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible()))[0].getLocalId(), id);
}

TEST_F(OpcuaDeviceModulesTest, ProcedureProp)
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

TEST_F(OpcuaDeviceModulesTest, DISABLED_ReferenceMethods)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto connectedSignal = signals[0];
    auto domainSignal = signals[1];

    auto inputPort = client.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_EQ(inputPort.getSignal(), connectedSignal);
    ASSERT_EQ(inputPort.getConnection(), connectedSignal.getConnections()[0]);
    ASSERT_EQ(connectedSignal.getDomainSignal(), domainSignal);

    // TODO: We have no related signals example. Should be added when we do.
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_DynamicSignalConfig)
{
    auto server = CreateServerInstance();
    auto serverDevice = server.addDevice("daqref://device0");
    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    clientSignal.setDomainSignal(clientSignals[1]);
    ASSERT_EQ(serverDevice.getSignals(search::Recursive(search::Visible()))[0].getDomainSignal().getLocalId(), clientSignal.getDomainSignal().getLocalId());
}

TEST_F(OpcuaDeviceModulesTest, FunctionBlocksOnClient)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    instance.setRootDevice("daqref://device1");

    const auto statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    instance.addServer("openDAQ OpcUa", nullptr);
    auto client = CreateClientInstance();

    ASSERT_GT(client.getDevices()[0].getFunctionBlocks().getCount(), (SizeT) 0);
}

TEST_F(OpcuaDeviceModulesTest, SdkPackageVersion)
{
    auto instance = InstanceBuilder().setDefaultRootDeviceInfo(DeviceInfo("", "dev", "custom")).build();
    instance.addServer("openDAQ OpcUa", nullptr);
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getInfo().getSdkVersion(), "custom");
}

TEST_F(OpcuaDeviceModulesTest, SdkPackageVersion1)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto info = client.getDevices()[0].getInfo();
    ASSERT_EQ(info.getPropertyValue("sdkVersion"), OPENDAQ_PACKAGE_VERSION);
}

// TODO: Add all examples of dynamic changes
// TODO: Add examples of all possible property object changes once rework is done
// TODO: Add examples of device topology, as well as signal/channel node trees in devices/fbs
// TODO: Add examples of streaming tests once streaming is available
