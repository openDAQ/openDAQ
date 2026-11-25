// ReSharper disable CppClangTidyModernizeAvoidBind
#include <opendaq/component.h>
#include <coreobjects/core_event_args.h>
#include <config_protocol/config_client_object_ptr.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/context_factory.h>
#include <config_protocol/config_client_device_impl.h>
#include <opendaq/packet_factory.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/component_type_private.h>
#include <coretypes/version_info_factory.h>
#include <coreobjects/user_factory.h>
#include <config_protocol/exceptions.h>
#include <opendaq/exceptions.h>
#include <testutils/testutils.h>
#include <opendaq/recorder_ptr.h>
#include <opendaq/module_impl.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/streaming_type_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;
using namespace std::placeholders;

class StreamingTypesModule : public Module
{
public:
    explicit StreamingTypesModule(const ContextPtr& ctx)
        : Module("Streaming types", VersionInfo(1, 0, 0), ctx, "StreamingTypes")
    {
    }

    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override
    {
        auto config = PropertyObject();
        config.addProperty(StringProperty("Host", "localhost"));
        const auto deviceType = DeviceTypeBuilder()
                                    .setId("mock_mirrored")
                                    .setName("Mock mirrored")
                                    .setConnectionStringPrefix("daqmirrored")
                                    .setDeviceKind(DeviceKind::Mirrored)
                                    .setDefaultConfig(config)
                                    .build();
        return Dict<IString, IDeviceType>({{"mock_mirrored", deviceType}});
    }

    DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes() override
    {
        const auto streamingB =
            StreamingTypeBuilder().setId("mock_streaming_b").setName("Mock streaming B").setConnectionStringPrefix("daqmockb").build();
        const auto streamingC =
            StreamingTypeBuilder().setId("mock_streaming_c").setName("Mock streaming C").setConnectionStringPrefix("daqmockc").build();
        return Dict<IString, IStreamingType>({{"mock_streaming_b", streamingB}, {"mock_streaming_c", streamingC}});
    }
};

class ClientStreamingTypesModule : public Module
{
public:
    explicit ClientStreamingTypesModule(const ContextPtr& ctx)
        : Module("Client streaming types", VersionInfo(1, 0, 0), ctx, "ClientStreamingTypes")
    {
    }

    DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes() override
    {
        const auto streamingB =
            StreamingTypeBuilder().setId("mock_streaming_b").setName("Mock streaming B").setConnectionStringPrefix("daqmockb").build();
        return Dict<IString, IStreamingType>({{"mock_streaming_b", streamingB}});
    }
};

class NewerServerDeviceImpl : public Device
{
public:
    NewerServerDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
    }

    AddDeviceConfigPtr onCreateAddDeviceConfig(const DeviceInfoPtr& info, const StringPtr& prefix) override
    {
        const auto addNewField = [](const PropertyObjectPtr& obj)
        {
            obj.addProperty(IntProperty("NewField", 1));
            obj.setPropertyValue("NewField", 2);
        };

        const auto addNewFieldWithTypeConfig = [&addNewField](const PropertyObjectPtr& obj)
        {
            addNewField(obj);
            addNewField(obj.getPropertyValue("TypeConfig"));
        };

        const PropertyObjectPtr config = Device::onCreateAddDeviceConfig(info, prefix);
        config.removeProperty("PrimaryAddressType");
        addNewFieldWithTypeConfig(config);

        const PropertyObjectPtr options = config.getPropertyValue("Streaming");
        addNewField(options);
        addNewFieldWithTypeConfig(options.getPropertyValue("mock_streaming_b"));
        options.removeProperty("StreamingType");
        options.addProperty(StringPropertyBuilder("StreamingType", "None")
                                .setSelectionValues(List<IString>("None", "mock_streaming_c", "mock_streaming_b", "new_streaming"))
                                .build());
        options.setPropertyValue("StreamingType", "new_streaming");

        const auto serializer = JsonSerializer();
        config.serialize(serializer);
        std::string json = serializer.getOutput();
        const std::string optionsType = R"("__type":"AddStreamingOptions",)";
        json.insert(json.find(optionsType) + optionsType.size(), R"("trimExceptions":["new_streaming"],)");
        return JsonDeserializer().deserialize(json, nullptr, nullptr).asPtr<IAddDeviceConfig>();
    }

    PropertyObjectPtr uploadedConfig;
    StringPtr loadedConfig;

protected:
    bool allowAddDevicesFromModules() override
    {
        return true;
    }

    DevicePtr onAddDevice(const StringPtr& /*connectionString*/, const PropertyObjectPtr& config) override
    {
        uploadedConfig = config;
        const auto device = createWithImplementation<IDevice, Device>(context, devices, "added");
        device.asPtr<IComponentPrivate>().setComponentConfig(config);
        devices.addItem(device);
        return device;
    }

    void updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context) override
    {
        if (obj.hasKey("Dev"))
            for (const auto& [_, savedDevice] : getSerializedItems(obj.readSerializedObject("Dev")))
                if (savedDevice.hasKey("ComponentConfig"))
                    loadedConfig = savedDevice.readSerializedObject("ComponentConfig").toJson();

        Device::updateObject(obj, context);
    }
};

class ConfigProtocolIntegrationTest : public Test
{
public:
    void SetUp() override
    {
        serverDevice = test_utils::createTestDevice();
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        clientContext = NullContext();
        connectClient();
    }

    void connectClient(uint16_t protocolVersion = GetLatestConfigProtocolVersion())
    {
        auto anonymousUser = User("", "");

        server = std::make_unique<ConfigProtocolServer>(
            serverDevice,
            std::bind(&ConfigProtocolIntegrationTest::serverNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(serverDevice.getContext()));

        client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                clientContext,
                std::bind(&ConfigProtocolIntegrationTest::sendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigProtocolIntegrationTest::sendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        clientDevice = client->connect(nullptr, protocolVersion);
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    static AddDeviceConfigPtr createAddDeviceConfigForPrefix(const StringPtr& prefix)
    {
        return AddDeviceConfig(DeviceTypeBuilder().setId(prefix).setName(prefix).setConnectionStringPrefix(prefix).build(), nullptr, False);
    }

    static StringPtr serializeComponent(const ComponentPtr& component)
    {
        const auto serializer = JsonSerializer(True);
        component.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }
    
    static DeviceInfoPtr createMirroredDeviceInfo()
    {
        const auto info = DeviceInfo("daqmirrored://device");
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();
        infoInternal.addServerCapability(ServerCapability("c", "c", ProtocolType::ConfigurationAndStreaming).setPrefix("daqmockc"));
        infoInternal.addServerCapability(ServerCapability("b", "b", ProtocolType::Streaming).setPrefix("daqmockb"));
        infoInternal.addServerCapability(ServerCapability("m", "m", ProtocolType::Configuration).setPrefix("daqmirrored"));
        return info;
    }

    NewerServerDeviceImpl* addNewerServerDevice() const
    {
        const ModuleManagerPtr manager = serverDevice.getContext().getModuleManager();
        manager.addModule(createWithImplementation<IModule, StreamingTypesModule>(NullContext()));

        const FolderConfigPtr devicesFolder = serverDevice.getItem("Dev");
        const auto device = createWithImplementation<IDevice, NewerServerDeviceImpl>(serverDevice.getContext(), devicesFolder, "newer");
        devicesFolder.addItem(device);
        return dynamic_cast<NewerServerDeviceImpl*>(device.getObject());
    }

    DevicePtr getClientNewerDevice() const
    {
        const FolderPtr devicesFolder = clientDevice.getItem("Dev");
        return devicesFolder.getItem("newer");
    }

    static void expectNewField(const PropertyObjectPtr& obj)
    {
        ASSERT_EQ(obj.getProperty("NewField").getDefaultValue(), 1);
        ASSERT_EQ(obj.getPropertyValue("NewField"), 2);
    }

    static void expectNewFieldWithTypeConfig(const PropertyObjectPtr& obj)
    {
        expectNewField(obj);
        expectNewField(obj.getPropertyValue("TypeConfig"));
    }

    static void expectNewerServerConfig(const PropertyObjectPtr& config)
    {
        ASSERT_TRUE(config.assigned());
        ASSERT_FALSE(config.hasProperty("PrimaryAddressType"));
        ASSERT_EQ(config.getPropertyValue("TypeConfig.Host"), "remote");
        expectNewFieldWithTypeConfig(config);

        const PropertyObjectPtr options = config.getPropertyValue("Streaming");
        expectNewField(options);
        ASSERT_EQ(options.getPropertyValue("StreamingType"), "mock_streaming_b");
        ASSERT_EQ(options.getProperty("StreamingType").getSelectionValues(), List<IString>("None", "mock_streaming_b", "new_streaming"));
        ASSERT_EQ(options.asPtr<IAddStreamingOptions>().getAvailableStreamingConfigs().getKeyList(),
                  List<IString>("mock_streaming_b", "new_streaming"));
        expectNewFieldWithTypeConfig(options.getPropertyValue("mock_streaming_b"));
    }

    static void expectNewerServerSavedValues(const StringPtr& json)
    {
        ASSERT_TRUE(json.assigned());
        const auto valuesOf = [](const SerializedObjectPtr& saved, const std::string& name)
        {
            return saved.readSerializedObject(name).readSerializedObject("propValues");
        };
        const auto expectValuesWithTypeConfig = [&valuesOf](const SerializedObjectPtr& values)
        {
            ASSERT_EQ(values.readInt("NewField"), 2);
            ASSERT_EQ(valuesOf(values, "TypeConfig").readInt("NewField"), 2);
        };
        const auto expectValues = [&](const SerializedObjectPtr& saved)
        {
            const auto values = saved.readSerializedObject("propValues");
            expectValuesWithTypeConfig(values);

            const auto options = valuesOf(values, "Streaming");
            ASSERT_EQ(options.readString("StreamingType"), "mock_streaming_b");
            ASSERT_EQ(options.readInt("NewField"), 2);
            expectValuesWithTypeConfig(valuesOf(options, "mock_streaming_b"));
        };
        JsonDeserializer().callCustomProc(expectValues, json);
    }
    // client handling
    PacketBuffer sendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
    }

    void sendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        server->processNoReplyRequest(requestPacket);
    }
    
    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        client->triggerNotificationPacket(notificationPacket);
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    ContextPtr clientContext;
    BaseObjectPtr notificationObj;

};

TEST_F(ConfigProtocolIntegrationTest, Connect)
{
    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    const auto clientDeviceSerialized = serializeComponent(clientDevice);
    ASSERT_EQ(serverDeviceSerialized, clientDeviceSerialized);
}

TEST_F(ConfigProtocolIntegrationTest, LockedAttributes)
{
    const auto toVector = [](const ListPtr<IString>& list)
    {
        std::vector<std::string> vec;
        for (const auto& attr : list)
            vec.push_back(attr);
        return vec;
    };

    // The device locks every attribute of its IO folder except "Active" when it is constructed
    const ComponentPtr serverIoFolder = serverDevice.getItem("IO");
    const ComponentPtr clientIoFolder = clientDevice.getItem("IO");
    ASSERT_THAT(toVector(serverIoFolder.getLockedAttributes()), ElementsAre("Description", "Name", "Visible"));
    ASSERT_THAT(toVector(clientIoFolder.getLockedAttributes()), ElementsAre("Description", "Name", "Visible"));
}

TEST_F(ConfigProtocolIntegrationTest, InputPortConnected)
{
    // visible input ports
    ASSERT_EQ(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              serverDevice.getDevices()[0].getSignals()[0]);

    ASSERT_EQ(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              clientDevice.getDevices()[0].getSignals()[0]);

    // hidden input ports
    ASSERT_EQ(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts(search::Not(search::Visible()))[0].getSignal(),
              serverDevice.getDevices()[0].getSignals()[0]);

    ASSERT_EQ(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts(search::Not(search::Visible()))[0].getSignal(),
              clientDevice.getDevices()[0].getSignals()[0]);
}

TEST_F(ConfigProtocolIntegrationTest, DisconnectAndConnectInputPortFromClient)
{
    ASSERT_TRUE(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    ASSERT_TRUE(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());

    const auto clientSignal = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal();

    clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].disconnect();
    ASSERT_FALSE(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    ASSERT_FALSE(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());

    clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].connect(clientSignal);
    ASSERT_TRUE(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    ASSERT_TRUE(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
}

TEST_F(ConfigProtocolIntegrationTest, DisconnectAndConnectInputPortFromServer)
{
    ASSERT_TRUE(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    ASSERT_TRUE(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    const auto serverSignal = serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal();

    serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].disconnect();

    ASSERT_FALSE(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    ASSERT_FALSE(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());

    serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].connect(serverSignal);
    ASSERT_TRUE(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
    ASSERT_TRUE(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal().assigned());
}

TEST_F(ConfigProtocolIntegrationTest, RemoteGlobalIds)
{
    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    ASSERT_EQ(clientDevice.asPtr<IConfigClientObject>(true).getRemoteGlobalId(), serverDevice.getGlobalId());
    ASSERT_EQ(clientDevice.getDevices()[0].asPtr<IConfigClientObject>(true).getRemoteGlobalId(), serverDevice.getDevices()[0].getGlobalId());
    ASSERT_EQ(clientDevice.getDevices()[0].getFunctionBlocks()[0].asPtr<IConfigClientObject>(true).getRemoteGlobalId(),
              serverDevice.getDevices()[0].getFunctionBlocks()[0].getGlobalId());
    ASSERT_EQ(clientDevice.getDevices()[0].getFunctionBlocks()[0].getSignals()[0].asPtr<IConfigClientObject>(true).getRemoteGlobalId(),
              serverDevice.getDevices()[0].getFunctionBlocks()[0].getSignals()[0].getGlobalId());
    ASSERT_EQ(clientDevice.getChannels()[0].getSignals()[0].asPtr<IConfigClientObject>(true).getRemoteGlobalId(),
              serverDevice.getChannels()[0].getSignals()[0].getGlobalId());
}

TEST_F(ConfigProtocolIntegrationTest, ConnectWithParent)
{
    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    
    ConfigProtocolClient<ConfigClientDeviceImpl> clientNew(
        clientContext,
        std::bind(&ConfigProtocolIntegrationTest::sendRequestAndGetReply, this, std::placeholders::_1),
        std::bind(&ConfigProtocolIntegrationTest::sendNoReplyRequest, this, std::placeholders::_1),
        nullptr,
        nullptr,
        nullptr
    );

    const auto parentComponent = Component(clientContext, nullptr, "cmp");

    const auto clientDevice = clientNew.connect(parentComponent);

    ASSERT_EQ(clientDevice.asPtr<IConfigClientObject>(true).getRemoteGlobalId(), serverDevice.getGlobalId());
    ASSERT_EQ(clientDevice.getGlobalId(), "/cmp/" + clientDevice.getLocalId().toStdString());
}

void checkComponentForConfigClientObject(const ComponentPtr& component)
{
    if (!component.supportsInterface<IConfigClientObject>())
        throw std::runtime_error(fmt::format("Component {} not a config client object", component.getGlobalId()));

    const auto folder = component.asPtrOrNull<IFolder>(true);
    if (folder.assigned())
        for (const auto& item: folder.getItems())
            checkComponentForConfigClientObject(item);
}

TEST_F(ConfigProtocolIntegrationTest, CheckConfigClientObject)
{
    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    checkComponentForConfigClientObject(clientDevice);
}

TEST_F(ConfigProtocolIntegrationTest, GetInitialPropertyValue)
{
    serverDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");

    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));

    const auto clientDeviceSerialized = serializeComponent(clientDevice);
    ASSERT_EQ(serverDeviceSerialized, clientDeviceSerialized);
}

TEST_F(ConfigProtocolIntegrationTest, GetInitialPropertyValuePropertySetter)
{
    serverDevice.getChannels()[0].getProperty("StrProp").setValue("SomeValue");

    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));

    const auto clientDeviceSerialized = serializeComponent(clientDevice);
    ASSERT_EQ(serverDeviceSerialized, clientDeviceSerialized);
}

TEST_F(ConfigProtocolIntegrationTest, SetPropertyValue)
{
    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");

    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));
}

TEST_F(ConfigProtocolIntegrationTest, SetPropertyValuePropertySetter)
{
    clientDevice.getChannels()[0].getProperty("StrProp").setValue("SomeValue");

    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));
}

TEST_F(ConfigProtocolIntegrationTest, SetProtectedPropertyValue)
{
    ASSERT_THROW(clientDevice.getChannels()[0].setPropertyValue("StrPropProtected", "SomeValue"), AccessDeniedException);

    clientDevice.getChannels()[0].asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrPropProtected", "SomeValue");

    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrPropProtected"), "SomeValue");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrPropProtected"), clientDevice.getChannels()[0].getPropertyValue("StrPropProtected"));
}

TEST_F(ConfigProtocolIntegrationTest, ClearPropertyValue)
{
    serverDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    clientDevice.getChannels()[0].clearPropertyValue("StrProp");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));
}

TEST_F(ConfigProtocolIntegrationTest, CallFuncProp)
{
    const auto serverCh = serverDevice.getChannels()[0];
    const auto funcProp =
        FunctionPropertyBuilder("FuncProp", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt))))
            .setReadOnly(True)
            .build();
    serverCh.addProperty(funcProp);

    serverCh.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("FuncProp", Function([](Int a, Int b) { return a + b; }));

    const auto clientFuncPropValue = clientDevice.getChannels()[0].getPropertyValue("FuncProp");

    Int r = clientFuncPropValue.call(2, 4);
    ASSERT_EQ(r, 6);
}

TEST_F(ConfigProtocolIntegrationTest, CallProcProp)
{
    const auto serverCh = serverDevice.getChannels()[0];
    const auto procProp =
        FunctionPropertyBuilder("ProcProp", ProcedureInfo())
            .setReadOnly(True)
            .build();
    serverCh.addProperty(procProp);

    bool called = false;
    serverCh.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("ProcProp", Procedure([&called] { called = true; }));

    const auto clientProcPropValue = clientDevice.getChannels()[0].getPropertyValue("ProcProp");

    clientProcPropValue.dispatch();
    ASSERT_TRUE(called);
}

TEST_F(ConfigProtocolIntegrationTest, GetDeviceInfo)
{
    const auto clientSubDevice = clientDevice.getDevices()[0];

    ASSERT_EQ(clientSubDevice.getInfo().getConnectionString(), "mock://dev1");
    ASSERT_EQ(clientSubDevice.getInfo().getName(), "MockDevice1");
    ASSERT_EQ(clientSubDevice.getInfo().getManufacturer(), "Testing");
}

TEST_F(ConfigProtocolIntegrationTest, DomainInfo)
{
    const auto clientSubDevice = clientDevice.getDevices()[0];

    ASSERT_EQ(clientSubDevice.getDomain().getOrigin(), "N/A");
    ASSERT_EQ(clientSubDevice.getDomain().getTickResolution(), Ratio(1, 100));
    ASSERT_EQ(clientSubDevice.getDomain().getUnit(), Unit("s", -1, "seconds", "time"));
}

TEST_F(ConfigProtocolIntegrationTest, GetTicksSinceOrigin)
{
    const auto clientSubDevice = clientDevice.getDevices()[0];
    ASSERT_EQ(clientSubDevice.getTicksSinceOrigin(), 0u);
    ASSERT_EQ(clientSubDevice.getTicksSinceOrigin(), 1u);
    ASSERT_EQ(clientSubDevice.getTicksSinceOrigin(), 2u);
}

TEST_F(ConfigProtocolIntegrationTest, GetAvailableFunctionBlockTypes)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];

    const auto fbTypes = clientSubDevice.getAvailableFunctionBlockTypes();
    ASSERT_EQ(fbTypes, serverSubDevice.getAvailableFunctionBlockTypes());
}

TEST_F(ConfigProtocolIntegrationTest, AddFunctionBlockNotFound)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];

    ASSERT_THROW(clientSubDevice.addFunctionBlock("someFb"), NotFoundException);
}

TEST_F(ConfigProtocolIntegrationTest, AddFunctionBlock)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];
    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);

    const auto globalId1 = fb.getGlobalId();
    const auto globalId2 = fb.asPtr<IConfigClientObject>().getRemoteGlobalId();
    const auto globalId3 = serverSubDevice.getFunctionBlocks()[1].getGlobalId();

    ASSERT_EQ(fb, clientSubDevice.getFunctionBlocks()[1]);

    ASSERT_EQ(fb.asPtr<IConfigClientObject>().getRemoteGlobalId(), serverSubDevice.getFunctionBlocks()[1].getGlobalId());
    ASSERT_EQ(fb.getSignals()[0].asPtr<IConfigClientObject>().getRemoteGlobalId(),
              serverSubDevice.getFunctionBlocks()[1].getSignals()[0].getGlobalId());
    ASSERT_EQ(fb.getSignals()[2].asPtr<IConfigClientObject>().getRemoteGlobalId(),
              serverSubDevice.getFunctionBlocks()[1].getSignals()[2].getGlobalId());
    ASSERT_EQ(fb.getSignals()[0].getDomainSignal(), fb.getSignals()[2]);
}

TEST_F(ConfigProtocolIntegrationTest, FunctionBlocksNested)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];
    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);
    auto types = fb.getAvailableFunctionBlockTypes();
    auto nestedFb = fb.addFunctionBlock(types.getKeyList()[0], config);
    ASSERT_TRUE(nestedFb.assigned());
    fb.removeFunctionBlock(nestedFb);

    ASSERT_TRUE(nestedFb.isRemoved());
    ASSERT_EQ(fb.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, AddFunctionBlockWithEvent)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];
    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);

    const auto globalId1 = fb.getGlobalId();
    const auto globalId2 = fb.asPtr<IConfigClientObject>().getRemoteGlobalId();
    const auto globalId3 = serverSubDevice.getFunctionBlocks()[1].getGlobalId();

    ASSERT_EQ(fb, clientSubDevice.getFunctionBlocks()[1]);

    ASSERT_EQ(fb.asPtr<IConfigClientObject>().getRemoteGlobalId(), serverSubDevice.getFunctionBlocks()[1].getGlobalId());
    ASSERT_EQ(fb.getSignals()[0].asPtr<IConfigClientObject>().getRemoteGlobalId(),
              serverSubDevice.getFunctionBlocks()[1].getSignals()[0].getGlobalId());
    ASSERT_EQ(fb.getSignals()[2].asPtr<IConfigClientObject>().getRemoteGlobalId(),
              serverSubDevice.getFunctionBlocks()[1].getSignals()[2].getGlobalId());
    ASSERT_EQ(fb.getSignals()[0].getDomainSignal(), fb.getSignals()[2]);
}

TEST_F(ConfigProtocolIntegrationTest, RemoveFunctionBlock)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];
    ASSERT_EQ(serverSubDevice.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(clientSubDevice.getFunctionBlocks().getCount(), 1u);

    ASSERT_NO_THROW(clientSubDevice.removeFunctionBlock(clientSubDevice.getFunctionBlocks()[0]));

    ASSERT_EQ(serverSubDevice.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(clientSubDevice.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, RemoveFunctionBlockWithEvent)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];
    ASSERT_EQ(serverSubDevice.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(clientSubDevice.getFunctionBlocks().getCount(), 1u);

    ASSERT_NO_THROW(clientSubDevice.removeFunctionBlock(clientSubDevice.getFunctionBlocks()[0]));

    ASSERT_EQ(serverSubDevice.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(clientSubDevice.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, RemoveFunctionBlockFromServer)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];
    ASSERT_EQ(serverSubDevice.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(clientSubDevice.getFunctionBlocks().getCount(), 1u);

    ASSERT_NO_THROW(serverSubDevice.removeFunctionBlock(clientSubDevice.getFunctionBlocks()[0]));

    ASSERT_EQ(serverSubDevice.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(clientSubDevice.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, GetInitialStructPropertyValue)
{
    ASSERT_EQ(serverDevice.getPropertyValue("StructProp"), clientDevice.getPropertyValue("StructProp"));
}

TEST_F(ConfigProtocolIntegrationTest, SetStructPropertyValue)
{
    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar1"}, {"Integer", 11}, {"Float", 5.223}});
    const auto structVal = Struct("FooStruct", structMembers, serverDevice.getContext().getTypeManager());
    serverDevice.setPropertyValue("StructProp", structVal);

    ASSERT_EQ(serverDevice.getPropertyValue("StructProp"), structVal);
    ASSERT_EQ(serverDevice.getPropertyValue("StructProp"), clientDevice.getPropertyValue("StructProp"));
}

TEST_F(ConfigProtocolIntegrationTest, DomainSignals)
{
    // visible signals
    ASSERT_EQ(serverDevice.getDevices()[0].getChannels()[0].getSignals()[0].getDomainSignal(),
              serverDevice.getDevices()[0].getChannels()[0].getSignals()[1]);

    ASSERT_EQ(clientDevice.getDevices()[0].getChannels()[0].getSignals()[0].getDomainSignal(),
              clientDevice.getDevices()[0].getChannels()[0].getSignals()[1]);

    // hidden signals
    ASSERT_EQ(serverDevice.getDevices()[0].getChannels()[0].getSignals(search::Not(search::Visible()))[0].getDomainSignal(),
              serverDevice.getDevices()[0].getChannels()[0].getSignals(search::Not(search::Visible()))[1]);

    ASSERT_EQ(clientDevice.getDevices()[0].getChannels()[0].getSignals(search::Not(search::Visible()))[0].getDomainSignal(),
              clientDevice.getDevices()[0].getChannels()[0].getSignals(search::Not(search::Visible()))[1]);
}

TEST_F(ConfigProtocolIntegrationTest, BeginEndUpdate)
{
    clientDevice.beginUpdate();
    clientDevice.setPropertyValue("StrProp", "SomeValue");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "-");
    clientDevice.endUpdate();
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "SomeValue");
}

TEST_F(ConfigProtocolIntegrationTest, BeginEndUpdateRecursive)
{
    clientDevice.beginUpdate();
    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    ASSERT_EQ(clientDevice.getChannels()[0].getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "-");
    clientDevice.endUpdate();
    ASSERT_EQ(clientDevice.getChannels()[0].getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "SomeValue");
}

TEST_F(ConfigProtocolIntegrationTest, BeginEndUpdateSubPropertyObject)
{
    const PropertyObjectPtr serverMockChild = serverDevice.getPropertyValue("MockChild");
    int state{0};

    serverMockChild.getOnEndUpdate() += [&state](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_EQ(state, 1);
        const auto propsChanged = args.getProperties();
        ASSERT_THAT(propsChanged, ElementsAre("NestedStringProperty"));
        state = 2;
    };

    serverMockChild.getOnPropertyValueWrite("NestedStringProperty") +=
        [&state](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
    {
        ASSERT_EQ(state, 0);
        auto prop = args.getProperty();
        ASSERT_TRUE(args.getIsUpdating());
        state = 1;
    };

    const PropertyObjectPtr clientMockChild = clientDevice.getPropertyValue("MockChild");
    ASSERT_EQ(clientMockChild.getPropertyValue("NestedStringProperty"), "String");

    clientMockChild.beginUpdate();
    clientMockChild.setPropertyValue("NestedStringProperty", "String1");
    clientMockChild.endUpdate();

    ASSERT_EQ(state, 2);
    ASSERT_EQ(clientMockChild.getPropertyValue("NestedStringProperty"), "String1");
}

TEST_F(ConfigProtocolIntegrationTest, BeginEndUpdateNestedPropertyObject)
{
    const PropertyObjectPtr serverMockChild = serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1");
    int state{0};

    serverMockChild.getOnEndUpdate() += [&state](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_EQ(state, 1);
        const auto propsChanged = args.getProperties();
        ASSERT_THAT(propsChanged, ElementsAre("String"));
        state = 2;
    };

    serverMockChild.getOnPropertyValueWrite("String") += [&state](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
    {
        ASSERT_EQ(state, 0);
        auto prop = args.getProperty();
        ASSERT_TRUE(args.getIsUpdating());
        state = 1;
    };

    const PropertyObjectPtr clientMockChild = clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1");
    ASSERT_EQ(clientMockChild.getPropertyValue("String"), "String");

    clientMockChild.beginUpdate();
    clientMockChild.setPropertyValue("String", "String1");
    clientMockChild.endUpdate();

    ASSERT_EQ(state, 2);
    ASSERT_EQ(clientMockChild.getPropertyValue("String"), "String1");
}

TEST_F(ConfigProtocolIntegrationTest, SetSignalNameAndDescriptionFromClient)
{
    const auto serverSignal = serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal();
    const auto clientSignal = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal();

    clientSignal.setName("SigName");

    ASSERT_EQ(clientSignal.getName(), "SigName");
    ASSERT_EQ(serverSignal.getName(), "SigName");
}

TEST_F(ConfigProtocolIntegrationTest, SetSignalNameAndDescriptionFromServer)
{
    const auto serverSignal = serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal();
    const auto clientSignal = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal();

    serverSignal.setName("SigName");

    ASSERT_EQ(clientSignal.getName(), "SigName");
    ASSERT_EQ(serverSignal.getName(), "SigName");
}

static void testMockPropertyObjectClass(const PropertyObjectPtr& obj)
{
    const auto className = obj.getClassName();
    ASSERT_EQ(className, "MockClass");
    ASSERT_TRUE(obj.hasProperty("MockString"));
    ASSERT_EQ(obj.getPropertyValue("MockString"), "String");

    ASSERT_TRUE(obj.hasProperty("MockChild"));
    const PropertyObjectPtr mockChild = obj.getPropertyValue("MockChild");
    ASSERT_TRUE(mockChild.hasProperty("NestedStringProperty"));
    ASSERT_EQ(mockChild.getPropertyValue("NestedStringProperty"), "String");
}

TEST_F(ConfigProtocolIntegrationTest, TestPropertyObjectClasses)
{
    testMockPropertyObjectClass(clientDevice);
    testMockPropertyObjectClass(clientDevice.getChannels()[0]);
    testMockPropertyObjectClass(clientDevice.getDevices()[0]);
    testMockPropertyObjectClass(clientDevice.getDevices()[0].getFunctionBlocks()[0]);
    testMockPropertyObjectClass(clientDevice.getDevices()[0].getChannels()[0]);
}

TEST_F(ConfigProtocolIntegrationTest, TestGetLastValue)
{
    const SignalConfigPtr serverSignal = serverDevice.getSignals()[0];
    const SignalConfigPtr clientSignal = clientDevice.getSignals()[0];

    auto dataPacket1 = DataPacket(serverSignal.getDescriptor(), 5);
    int64_t* data1 = static_cast<int64_t*>(dataPacket1.getData());
    data1[4] = 4;
    serverSignal.sendPacket(dataPacket1);

    auto lastValue1 = clientSignal.getLastValue();
    IntegerPtr integerPtr;
    ASSERT_NO_THROW(integerPtr = lastValue1.asPtr<IInteger>());
    ASSERT_EQ(integerPtr, 4);

    auto dataPacket2 = DataPacket(serverSignal.getDescriptor(), 2);
    int64_t* data2 = static_cast<int64_t*>(dataPacket2.getData());
    data2[1] = 7;
    serverSignal.sendPacket(dataPacket2);

    auto lastValue2 = clientSignal.getLastValue();
    IntegerPtr integerPtr2;
    ASSERT_NO_THROW(integerPtr2 = lastValue2.asPtr<IInteger>());
    ASSERT_EQ(integerPtr2, 7);
}

TEST_F(ConfigProtocolIntegrationTest, TestGetLastValueWithoutTimestamp)
{
    const auto serverSignals = serverDevice.getSignals(search::Recursive(search::Custom(Function(
        [](const BaseObjectPtr& obj)
        {
            auto signal = obj.template asPtr<ISignal>();
            return (!signal.getDomainSignal().assigned() && signal.getDescriptor().assigned() &&
                    (signal.getDescriptor().getSampleType() == SampleType::Int64));
        }))));
    ASSERT_FALSE(serverSignals.getCount() == 0);
    SignalConfigPtr serverSignal = serverSignals[0];

    const auto clientSignals = clientDevice.getSignals(search::Recursive(search::Custom(Function(
        [id = serverSignal.getGlobalId()](const BaseObjectPtr& obj) { return obj.template asPtr<ISignal>().getGlobalId() == id; }))));
    ASSERT_FALSE(clientSignals.getCount() == 0);
    SignalConfigPtr clientSignal = clientSignals[0];

    const auto senderAndChecker = [&](uint64_t sampleCount, int64_t lastValue)
    {
        auto dataPacket = DataPacket(serverSignal.getDescriptor(), sampleCount);

        static_cast<int64_t*>(dataPacket.getData())[sampleCount - 1] = lastValue;

        serverSignal.sendPacket(dataPacket);

        BaseObjectPtr value;
        BaseObjectPtr timestamp;
        ASSERT_NO_THROW(timestamp = clientSignal.getLastValueWithTimestamp(value));

        IntegerPtr integerPtr;
        ASSERT_NO_THROW(integerPtr = value.asPtr<IInteger>());
        ASSERT_EQ(integerPtr, lastValue);
        ASSERT_FALSE(timestamp.assigned());
    };
    senderAndChecker(5, 987);
    senderAndChecker(2, 1000);
}

TEST_F(ConfigProtocolIntegrationTest, TestGetLastValueWithTimestamp)
{
    const auto serverSignals = serverDevice.getSignals(search::Recursive(search::Custom(Function(
        [](const BaseObjectPtr& obj)
        {
            auto signal = obj.template asPtr<ISignal>();
            return (signal.getDomainSignal().assigned() && signal.getDomainSignal().getDescriptor().assigned() &&
                    signal.getName() == "sig1");
        }))));
    ASSERT_FALSE(serverSignals.getCount() == 0);
    SignalConfigPtr serverSignal = serverSignals[0];
    SignalConfigPtr serverDomainSignal = serverSignal.getDomainSignal();
    ASSERT_TRUE(serverDomainSignal.getDescriptor().assigned());
    ASSERT_TRUE(serverDomainSignal.getDescriptor().getUnit().assigned());
    ASSERT_TRUE(serverDomainSignal.getDescriptor().getUnit().getSymbol() == "s");
    ASSERT_TRUE(serverDomainSignal.getDescriptor().getOrigin().assigned());

    const auto clientSignals = clientDevice.getSignals(search::Recursive(search::Custom(Function(
        [id = serverSignal.getGlobalId()](const BaseObjectPtr& obj) { return obj.template asPtr<ISignal>().getGlobalId() == id; }))));
    ASSERT_FALSE(clientSignals.getCount() == 0);
    SignalConfigPtr clientSignal = clientSignals[0];

    const auto senderAndChecker = [&](uint64_t sampleCount, int64_t lastValue, uint64_t lastTs)
    {
        auto domainDataPacket = DataPacket(serverSignal.getDomainSignal().getDescriptor(), sampleCount);
        auto dataPacket = DataPacketWithDomain(domainDataPacket, serverSignal.getDescriptor(), sampleCount);

        static_cast<int64_t*>(dataPacket.getData())[sampleCount - 1] = lastValue;
        static_cast<uint64_t*>(domainDataPacket.getData())[sampleCount - 1] = lastTs;

        serverDomainSignal.sendPacket(domainDataPacket);
        serverSignal.sendPacket(dataPacket);

        BaseObjectPtr value;
        BaseObjectPtr timestamp;
        ASSERT_NO_THROW(timestamp = clientSignal.getLastValueWithTimestamp(value));

        IntegerPtr integerPtr;
        ASSERT_NO_THROW(integerPtr = value.asPtr<IInteger>());
        ASSERT_EQ(integerPtr, lastValue);
        ASSERT_TRUE(timestamp.assigned());
        ASSERT_EQ(timestamp.asPtr<IInteger>(), lastTs * 1'000'000);
    };
    senderAndChecker(5, 987, 123456);
    senderAndChecker(2, 1000, 123789);
}

TEST_F(ConfigProtocolIntegrationTest, DeviceInfoChanges)
{
    const auto serverSubDevice = serverDevice.getDevices()[1];
    const auto clientSubDevice = clientDevice.getDevices()[1];

    const auto serverDeviceInfo = serverSubDevice.getInfo();
    const auto clientDeviceInfo = clientSubDevice.getInfo();

    // set fields on server
    serverSubDevice.setPropertyValue("userName", "new_name");
    serverSubDevice.setPropertyValue("location", "new_location");

    ASSERT_EQ("new_name", serverDeviceInfo.getPropertyValue("userName"));
    ASSERT_EQ("new_location", serverDeviceInfo.getLocation());

    ASSERT_EQ(serverDeviceInfo.getPropertyValue("userName"), clientDeviceInfo.getPropertyValue("userName"));
    ASSERT_EQ(serverDeviceInfo.getLocation(), clientDeviceInfo.getLocation());

    // set fields on client
    clientSubDevice.setPropertyValue("userName", "new_client_name");
    clientSubDevice.setPropertyValue("location", "new_client_location");

    ASSERT_EQ("new_client_name", clientDeviceInfo.getPropertyValue("userName"));
    ASSERT_EQ("new_client_location", clientDeviceInfo.getLocation());

    ASSERT_EQ(serverDeviceInfo.getPropertyValue("userName"), clientDeviceInfo.getPropertyValue("userName"));
    ASSERT_EQ(serverDeviceInfo.getLocation(), clientDeviceInfo.getLocation());
}

TEST_F(ConfigProtocolIntegrationTest, DeviceInfoChangeableField)
{
    const auto serverSubDevice = serverDevice.getDevices()[1];
    const auto clientSubDevice = clientDevice.getDevices()[1];

    const auto serverDeviceInfo = serverSubDevice.getInfo();
    const auto clientDeviceInfo = clientSubDevice.getInfo();

    ASSERT_EQ(serverDeviceInfo.getPropertyValue("TestChangeableField"), clientDeviceInfo.getPropertyValue("TestChangeableField"));

    serverDeviceInfo.setPropertyValue("TestChangeableField", "new_value");
    ASSERT_EQ("new_value", serverDeviceInfo.getPropertyValue("TestChangeableField"));
    ASSERT_EQ("new_value", clientDeviceInfo.getPropertyValue("TestChangeableField"));

    clientDeviceInfo.setPropertyValue("TestChangeableField", "new_value_2");
    ASSERT_EQ("new_value_2", serverDeviceInfo.getPropertyValue("TestChangeableField"));
    ASSERT_EQ("new_value_2", clientDeviceInfo.getPropertyValue("TestChangeableField"));
}

TEST_F(ConfigProtocolIntegrationTest, DeviceInfoNotChangeableField)
{
    const auto serverSubDevice = serverDevice.getDevices()[1];
    const auto clientSubDevice = clientDevice.getDevices()[1];

    const auto serverDeviceInfo = serverSubDevice.getInfo();
    const auto clientDeviceInfo = clientSubDevice.getInfo();

    ASSERT_EQ("manufacturer", serverDeviceInfo.getManufacturer());
    ASSERT_EQ("manufacturer", clientDeviceInfo.getManufacturer());
    
    {
        ASSERT_ANY_THROW(serverDeviceInfo.setPropertyValue("manufacturer", "server_manufacturer"));
        ASSERT_EQ("manufacturer", serverDeviceInfo.getManufacturer());
        ASSERT_EQ(serverDeviceInfo.getManufacturer(), clientDeviceInfo.getManufacturer());

        serverDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("manufacturer", "server_manufacturer_2");
        ASSERT_EQ("server_manufacturer_2", serverDeviceInfo.getManufacturer());
        ASSERT_EQ(serverDeviceInfo.getManufacturer(), clientDeviceInfo.getManufacturer());

        ASSERT_ANY_THROW(serverDeviceInfo.asPtr<IDeviceInfoConfig>(true).setManufacturer("server_manufacturer_3"));
        ASSERT_EQ("server_manufacturer_2", serverDeviceInfo.getManufacturer());
        ASSERT_EQ(serverDeviceInfo.getManufacturer(), clientDeviceInfo.getManufacturer());
    }

    {
        ASSERT_ANY_THROW(clientDeviceInfo.setPropertyValue("manufacturer", "client_manufacturer"));
        ASSERT_EQ("server_manufacturer_2", serverDeviceInfo.getManufacturer());
        ASSERT_EQ(serverDeviceInfo.getManufacturer(), clientDeviceInfo.getManufacturer());

        clientDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("manufacturer", "client_manufacturer_2");
        ASSERT_EQ("server_manufacturer_2", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("client_manufacturer_2", clientDeviceInfo.getManufacturer());

        ASSERT_ANY_THROW(clientDeviceInfo.asPtr<IDeviceInfoConfig>(true).setManufacturer("client_manufacturer_3"));
        ASSERT_EQ("server_manufacturer_2", serverDeviceInfo.getManufacturer());
        ASSERT_EQ("client_manufacturer_2", clientDeviceInfo.getManufacturer());
    }
}

TEST_F(ConfigProtocolIntegrationTest, OnWriteReadEvents)
{
    ASSERT_THROW(clientDevice.getOnPropertyValueWrite("location"), NativeClientCallNotAvailableException);
    ASSERT_THROW(clientDevice.getOnPropertyValueRead("location"), NativeClientCallNotAvailableException);
    ASSERT_THROW(clientDevice.getOnAnyPropertyValueWrite(), NativeClientCallNotAvailableException);
    ASSERT_THROW(clientDevice.getOnAnyPropertyValueRead(), NativeClientCallNotAvailableException);
}

TEST_F(ConfigProtocolIntegrationTest, AcceptsSignal)
{
    auto clientSignal = clientDevice.getSignals()[0];
    daq::Bool clientAcceptsClientSignal = False;
    ASSERT_NO_THROW(clientAcceptsClientSignal =
                        clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].acceptsSignal(clientSignal));
    ASSERT_TRUE(clientAcceptsClientSignal);

    auto parentlessSignal = Signal(clientSignal.getContext(), nullptr, "test");
    daq::Bool clientAcceptsParentlessSignal = False;
    ASSERT_NO_THROW(clientAcceptsParentlessSignal =
                        clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].acceptsSignal(parentlessSignal));
    ASSERT_TRUE(clientAcceptsParentlessSignal);

    // such connection likely creates unsafe loopback with undefined behavior, but signal is still accepted
    auto serverSignal = serverDevice.getSignals()[0];
    daq::Bool clientAcceptsServerSignal = False;
    ASSERT_NO_THROW(clientAcceptsServerSignal =
                    clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].acceptsSignal(serverSignal));
    ASSERT_TRUE(clientAcceptsServerSignal);
}

TEST_F(ConfigProtocolIntegrationTest, AcceptsSignals)
{
    auto clientSignal = clientDevice.getSignals()[0];
    auto parentlessSignal = Signal(clientSignal.getContext(), nullptr, "test");
    // such connection likely creates unsafe loopback with undefined behavior, but signal is still accepted
    auto serverSignal = serverDevice.getSignals()[0];

    auto signalList = List<ISignal>();
    signalList.pushBack(clientSignal);
    signalList.pushBack(parentlessSignal);
    signalList.pushBack(serverSignal);

    daq::ListPtr<daq::IBoolean> acceptanceList = nullptr;
    ASSERT_NO_THROW(acceptanceList =
                        clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].acceptsSignals(signalList));
    ASSERT_TRUE(acceptanceList.assigned());
    ASSERT_TRUE(acceptanceList.getCount() == 3);
    ASSERT_TRUE(acceptanceList[0]);
    ASSERT_TRUE(acceptanceList[1]);
    ASSERT_TRUE(acceptanceList[2]);
}

TEST_F(ConfigProtocolIntegrationTest, GetAvailableDevices)
{
    auto availableDevicesServer = serverDevice.getAvailableDevices();
    auto availableDevicesClient = clientDevice.getAvailableDevices();

    ASSERT_EQ(availableDevicesClient.getCount(), 4u);

    auto c = availableDevicesClient[2];
    auto s = availableDevicesServer[2];

    ASSERT_EQ(c.getName(), "AvailableMockDevice2");
    ASSERT_EQ(c.getConnectionString(), "mock://available_dev2");
    ASSERT_EQ(c.getManufacturer(), "Testing");

    ASSERT_EQ(c.getName(), s.getName());
    ASSERT_EQ(c.getConnectionString(), s.getConnectionString());
    ASSERT_EQ(c.getManufacturer(), s.getManufacturer());
    ASSERT_EQ(c.getDeviceType().getId(), "mockAvailableDev");
    ASSERT_EQ(c.getDeviceType().getConnectionStringPrefix(), s.getDeviceType().getConnectionStringPrefix());
}

void addDeviceTest(DevicePtr clientDevice, DevicePtr serverDevice, bool testAddDevicesMethod = false)
{
    ASSERT_EQ(clientDevice.getDevices().getCount(), 2u);
    ASSERT_EQ(serverDevice.getDevices().getCount(), 2u);

    DevicePtr dev;
    if (testAddDevicesMethod)
    {
        auto connectionArgs =
            Dict<IString, IPropertyObject>(
                {
                    {"mock://test", nullptr},
                    {"unknown://unknown", nullptr}
                }
            );
        auto errCodes = Dict<IString, IInteger>();
        auto errorInfos = Dict<IString, IErrorInfo>();
        DictPtr<IString, IDevice> devices;
        ASSERT_EQ(clientDevice->addDevices(&devices, connectionArgs, errCodes, errorInfos),
                  OPENDAQ_PARTIAL_SUCCESS);

        ASSERT_EQ(devices.getCount(), 2u);
        ASSERT_EQ(errCodes.getCount(), 2u);
        ASSERT_EQ(errorInfos.getCount(), 2u);

        dev = devices.get("mock://test");
        ASSERT_EQ(errCodes.get("mock://test"), OPENDAQ_SUCCESS);
        ASSERT_FALSE(errorInfos.get("mock://test").assigned());

        ASSERT_FALSE(devices.get("unknown://unknown").assigned());
        ASSERT_EQ(errCodes.get("unknown://unknown"), OPENDAQ_ERR_NOTFOUND);
        ASSERT_TRUE(errorInfos.get("unknown://unknown").assigned());
    }
    else
    {
        ASSERT_NO_THROW(dev = clientDevice.addDevice("mock://test"));
    }

    ASSERT_NE(dev, nullptr);

    ASSERT_EQ(clientDevice.getDevices().getCount(), 3u);
    ASSERT_EQ(serverDevice.getDevices().getCount(), 3u);

    auto newDevCli = clientDevice.getDevices()[2];
    ASSERT_EQ(dev, newDevCli);
    auto newDevSer = serverDevice.getDevices()[2];

    ASSERT_EQ(newDevCli.getGlobalId(), "/root_dev/Dev/newDevice");
    ASSERT_EQ(newDevCli.asPtr<IConfigClientObject>().getRemoteGlobalId(), "/root_dev/Dev/newDevice");
    ASSERT_EQ(newDevSer.getGlobalId(), "/root_dev/Dev/newDevice");

    auto newCliFB = newDevCli.getDevices()[0].getFunctionBlocks()[0];
    auto newSerFB = newDevSer.getDevices()[0].getFunctionBlocks()[0];

    auto domainClient = newCliFB.getSignals()[0].getDomainSignal();
    ASSERT_NE(domainClient, nullptr);

    auto serverClient = newSerFB.getSignals()[0].getDomainSignal();
    ASSERT_NE(serverClient, nullptr);

    auto connClient = newCliFB.getInputPorts()[0].getConnection();
    ASSERT_NE(connClient, nullptr);

    auto connServer = newSerFB.getInputPorts()[0].getConnection();
    ASSERT_NE(connServer, nullptr);
}

TEST_F(ConfigProtocolIntegrationTest, AddDeviceDisableCoreEventTrigger)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    addDeviceTest(clientDevice, serverDevice);
}

TEST_F(ConfigProtocolIntegrationTest, AddDeviceCoreEventTrigger)
{
    addDeviceTest(clientDevice, serverDevice);
}

TEST_F(ConfigProtocolIntegrationTest, AddDevicesDisableCoreEventTrigger)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    addDeviceTest(clientDevice, serverDevice, true);
}

TEST_F(ConfigProtocolIntegrationTest, AddDevicesCoreEventTrigger)
{
    addDeviceTest(clientDevice, serverDevice, true);
}

void removeDeviceTest(DevicePtr clientDevice, DevicePtr serverDevice)
{
    ASSERT_EQ(clientDevice.getDevices().getCount(), 2u);
    ASSERT_EQ(serverDevice.getDevices().getCount(), 2u);

    clientDevice.removeDevice(clientDevice.getDevices()[0]);

    ASSERT_EQ(clientDevice.getDevices().getCount(), 1u);
    ASSERT_EQ(serverDevice.getDevices().getCount(), 1u);
}

TEST_F(ConfigProtocolIntegrationTest, RemoveDeviceDisableCoreEventTrigger)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    removeDeviceTest(clientDevice, serverDevice);
}

TEST_F(ConfigProtocolIntegrationTest, RemoveDeviceCoreEventTrigger)
{
    removeDeviceTest(clientDevice, serverDevice);
}

TEST_F(ConfigProtocolIntegrationTest, GetAvailableDeviceTypes)
{
    auto availableDeviceTypesServer = serverDevice.getDevices()[0].getAvailableDeviceTypes();
    auto availableDeviceTypesClient = clientDevice.getDevices()[0].getAvailableDeviceTypes();

    ASSERT_EQ(availableDeviceTypesClient.getCount(), 1u);

    auto c = availableDeviceTypesClient.get("mockDev1");
    auto s = availableDeviceTypesServer.get("mockDev1");

    ASSERT_EQ(c.getId(), "mockDev1");
    ASSERT_EQ(c.getName(), "MockDev1");
    ASSERT_EQ(c.getDescription(), "Mock Device 1");
    ASSERT_EQ(c.getConnectionStringPrefix(), "prefix");

    ASSERT_EQ(c.getId(), s.getId());
    ASSERT_EQ(c.getName(), s.getName());
    ASSERT_EQ(c.getDescription(), s.getDescription());
    ASSERT_EQ(c.getConnectionStringPrefix(), s.getConnectionStringPrefix());
    ASSERT_EQ(c.getDeviceKind(), DeviceKind::Mirrored);
    ASSERT_EQ(s.getDeviceKind(), DeviceKind::Mirrored);
}

TEST_F(ConfigProtocolIntegrationTest, CreateAddDeviceConfig)
{
    const ModuleManagerPtr manager = serverDevice.getContext().getModuleManager();
    manager.addModule(createWithImplementation<IModule, StreamingTypesModule>(NullContext()));

    const auto info = createMirroredDeviceInfo();

    const auto serverConfig = serverDevice.createAddDeviceConfig(info, "daqmirrored");
    const auto clientConfig = clientDevice.createAddDeviceConfig(info, "daqmirrored");

    const auto serverOptions = serverConfig.getStreamingOptions();
    const auto clientOptions = clientConfig.getStreamingOptions();
    ASSERT_EQ(clientConfig.getDeviceType().getId(), serverConfig.getDeviceType().getId());
    ASSERT_EQ(clientOptions.getAvailableStreamingConfigs().getKeyList(), serverOptions.getAvailableStreamingConfigs().getKeyList());
    ASSERT_EQ(clientOptions.getStreamingType(), serverOptions.getStreamingType());
    ASSERT_FALSE(clientConfig.getTypeConfig().supportsInterface<IConfigClientObject>());
}

TEST_F(ConfigProtocolIntegrationTest, CreateAddDeviceConfigOnOldServer)
{
    const ModuleManagerPtr serverManager = serverDevice.getContext().getModuleManager();
    serverManager.addModule(createWithImplementation<IModule, StreamingTypesModule>(NullContext()));

    clientContext = Context(nullptr, Logger(), TypeManager(), ModuleManager("[[none]]"));
    const ModuleManagerPtr clientManager = clientContext.getModuleManager();
    clientManager.addModule(createWithImplementation<IModule, ClientStreamingTypesModule>(NullContext()));
    connectClient(25);

    const auto config = clientDevice.createAddDeviceConfig(createMirroredDeviceInfo(), "daqmirrored");
    ASSERT_EQ(config.getDeviceType().getId(), "mock_mirrored");
    ASSERT_EQ(config.getStreamingOptions().getAvailableStreamingConfigs().getKeyList(), List<IString>("mock_streaming_b"));
}

TEST_F(ConfigProtocolIntegrationTest, CreateAddDeviceConfigErrorsReachClient)
{
    ASSERT_THROW(clientDevice.createAddDeviceConfig(DeviceInfo("unknown://device")), NotFoundException);
    ASSERT_THROW(clientDevice.getDevices()[0].createAddDeviceConfig(DeviceInfo("unknown://device")), NotSupportedException);
}

TEST_F(ConfigProtocolIntegrationTest, AddDeviceResolvesDaqConnectionStringOnOldServer)
{
    const auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());
    connectClient(25);

    ASSERT_THROW(clientDevice.addDevice("daq://Testing_smart?Opt=1", createAddDeviceConfigForPrefix("daqmockx")), NotFoundException);
    ASSERT_EQ(mock->lastAddDeviceConnectionString, "mock://test?Opt=1");
}

TEST_F(ConfigProtocolIntegrationTest, AddDevicesResolvesDaqConnectionStringsOnOldServer)
{
    connectClient(25);

    const auto config = createAddDeviceConfigForPrefix("daqmockx");
    const auto connectionArgs =
        Dict<IString, IPropertyObject>({{"mock://test", nullptr}, {"daq://Testing_smart", config}, {"daq://Testing_missing", config}});
    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();
    DictPtr<IString, IDevice> devices;
    ASSERT_EQ(clientDevice->addDevices(&devices, connectionArgs, errCodes, errorInfos), OPENDAQ_PARTIAL_SUCCESS);

    ASSERT_TRUE(devices.get("mock://test").assigned());
    ASSERT_EQ(errCodes.get("mock://test"), OPENDAQ_SUCCESS);
    ASSERT_FALSE(errorInfos.get("mock://test").assigned());

    ASSERT_FALSE(devices.get("daq://Testing_smart").assigned());
    ASSERT_EQ(errCodes.get("daq://Testing_smart"), OPENDAQ_ERR_ALREADYEXISTS);
    ASSERT_TRUE(errorInfos.get("daq://Testing_smart").assigned());

    ASSERT_FALSE(devices.get("daq://Testing_missing").assigned());
    ASSERT_EQ(errCodes.get("daq://Testing_missing"), OPENDAQ_ERR_NOTFOUND);
    ASSERT_TRUE(errorInfos.get("daq://Testing_missing").assigned());

    const auto unresolvedArgs = Dict<IString, IPropertyObject>({{"daq://Testing_missing", config}});
    ASSERT_EQ(clientDevice->addDevices(&devices, unresolvedArgs, errCodes, errorInfos), OPENDAQ_ERR_GENERALERROR);
    daqClearErrorInfo();
    ASSERT_FALSE(devices.get("daq://Testing_missing").assigned());
    ASSERT_EQ(errCodes.get("daq://Testing_missing"), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(ConfigProtocolIntegrationTest, AddDeviceSendsDaqConnectionStringUnchanged)
{
    const auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());

    ASSERT_THROW(clientDevice.addDevice("daq://Testing_smart", createAddDeviceConfigForPrefix("daqmockx")), NotFoundException);
    ASSERT_EQ(mock->lastAddDeviceConnectionString, "daq://Testing_smart");

    connectClient(25);
    mock->lastAddDeviceConnectionString = nullptr;
    ASSERT_THROW(clientDevice.addDevice("daq://Testing_smart"), NotFoundException);
    ASSERT_EQ(mock->lastAddDeviceConnectionString, "daq://Testing_smart");
}

TEST_F(ConfigProtocolIntegrationTest, DeviceTypesModuleInfo)
{
    auto availableDeviceTypesServer = serverDevice.getDevices()[0].getAvailableDeviceTypes();
    auto availableDeviceTypesClient = clientDevice.getDevices()[0].getAvailableDeviceTypes();

    auto s = availableDeviceTypesServer.get("mockDev1");
    auto c = availableDeviceTypesClient.get("mockDev1");

    auto moduleInfoS = s.getModuleInfo();
    auto moduleInfoC = c.getModuleInfo();

    ASSERT_EQ(moduleInfoS.getId(), "module_id");
    ASSERT_EQ(moduleInfoS.getName(), "module_name");
    ASSERT_EQ(moduleInfoS.getVersionInfo().getMajor(), 5u);
    ASSERT_EQ(moduleInfoS.getVersionInfo().getMinor(), 6u);
    ASSERT_EQ(moduleInfoS.getVersionInfo().getPatch(), 7u);

    ASSERT_EQ(moduleInfoC.getId(), "module_id");
    ASSERT_EQ(moduleInfoC.getName(), "module_name");
    ASSERT_EQ(moduleInfoC.getVersionInfo().getMajor(), 5u);
    ASSERT_EQ(moduleInfoC.getVersionInfo().getMinor(), 6u);
    ASSERT_EQ(moduleInfoC.getVersionInfo().getPatch(), 7u);
}

TEST_F(ConfigProtocolIntegrationTest, FunctionBlockTypesModuleInfo)
{
    auto availableFunctionBlockTypesServer = serverDevice.getDevices()[0].getAvailableFunctionBlockTypes();
    auto availableFunctionBlockTypesClient = clientDevice.getDevices()[0].getAvailableFunctionBlockTypes();

    auto s = availableFunctionBlockTypesServer.get("mockfb1");
    auto c = availableFunctionBlockTypesClient.get("mockfb1");

    auto moduleInfoS = s.getModuleInfo();
    auto moduleInfoC = c.getModuleInfo();

    ASSERT_EQ(moduleInfoS.getId(), "module_id");
    ASSERT_EQ(moduleInfoS.getName(), "module_name");
    ASSERT_EQ(moduleInfoS.getVersionInfo().getMajor(), 5u);
    ASSERT_EQ(moduleInfoS.getVersionInfo().getMinor(), 6u);
    ASSERT_EQ(moduleInfoS.getVersionInfo().getPatch(), 7u);

    ASSERT_EQ(moduleInfoC.getId(), "module_id");
    ASSERT_EQ(moduleInfoC.getName(), "module_name");
    ASSERT_EQ(moduleInfoC.getVersionInfo().getMajor(), 5u);
    ASSERT_EQ(moduleInfoC.getVersionInfo().getMinor(), 6u);
    ASSERT_EQ(moduleInfoC.getVersionInfo().getPatch(), 7u);
}

TEST_F(ConfigProtocolIntegrationTest, RecorderFunctionBlock)
{
    const auto clientSubDevice = clientDevice.getDevices()[0];

    const auto recorderFb = clientSubDevice.addFunctionBlock("mockrecorder1");
    const RecorderPtr recorderPtr = recorderFb.asPtrOrNull<IRecorder>();

    ASSERT_TRUE(recorderPtr.assigned());
    ASSERT_FALSE(recorderPtr.getIsRecording());

    ASSERT_NO_THROW(recorderPtr.startRecording());
    ASSERT_TRUE(recorderPtr.getIsRecording());
    ASSERT_THROW(recorderPtr.startRecording(), InvalidStateException);

    ASSERT_NO_THROW(recorderPtr.stopRecording());
    ASSERT_FALSE(recorderPtr.getIsRecording());
    ASSERT_THROW(recorderPtr.stopRecording(), InvalidStateException);
}

TEST_F(ConfigProtocolIntegrationTest, ComponentConfig)
{
    auto deviceComponentConfig = clientDevice.getDevices()[0].asPtr<IComponentPrivate>().getComponentConfig();
    auto fbComponentConfig = clientDevice.getDevices()[0].getFunctionBlocks()[0].asPtr<IComponentPrivate>().getComponentConfig();
    ASSERT_TRUE(deviceComponentConfig.assigned());
    ASSERT_TRUE(fbComponentConfig.assigned());
    ASSERT_TRUE(deviceComponentConfig.hasProperty("TestProp"));
    ASSERT_TRUE(fbComponentConfig.hasProperty("TestProp"));
}

TEST_F(ConfigProtocolIntegrationTest, ClientDeviceHasAddDeviceConfigOfServerDevice)
{
    const auto deviceType = DeviceTypeBuilder().setId("dev").setName("dev").setConnectionStringPrefix("dev").build();
    const auto s1 = StreamingTypeBuilder().setId("s1").setName("s1").setConnectionStringPrefix("s1").build();
    const auto s2 = StreamingTypeBuilder().setId("s2").setName("s2").setConnectionStringPrefix("s2").build();
    const auto streamingTypes = Dict<IString, IStreamingType>({{"s1", s1}, {"s2", s2}});
    const auto addConfig = AddDeviceConfig(deviceType, streamingTypes, False);
    addConfig.getStreamingOptions().setStreamingType("s1");

    const auto serverSubDevice = serverDevice.getDevices()[0];
    serverSubDevice.asPtr<IDevicePrivate>().setAsRoot();
    serverSubDevice.asPtr<IComponentPrivate>().setComponentConfig(addConfig);

    connectClient(26);
    const auto config = clientDevice.getDevices()[0].getAddDeviceConfig();
    ASSERT_TRUE(config.assigned());
    ASSERT_EQ(config.getDeviceType().getId(), "dev");
    ASSERT_EQ(config.getStreamingOptions().getStreamingConfig().getStreamingType().getId(), "s1");

    connectClient(25);
    ASSERT_FALSE(clientDevice.getDevices()[0].getAddDeviceConfig().assigned());
}

TEST_F(ConfigProtocolIntegrationTest, AddDeviceConfigOfNewerServerSurvivesRoundTrip)
{
    const auto serverNewer = addNewerServerDevice();
    const auto clientNewer = getClientNewerDevice();

    const auto config = clientNewer.createAddDeviceConfig(createMirroredDeviceInfo(), "daqmirrored");
    const auto options = config.getStreamingOptions();
    ASSERT_EQ(options.asPtr<IPropertyObject>().getProperty("StreamingType").getSelectionValues(),
              List<IString>("None", "mock_streaming_c", "mock_streaming_b", "new_streaming"));
    ASSERT_EQ(options.getStreamingType(), "new_streaming");
    ASSERT_FALSE(options.getStreamingConfig().assigned());
    options.setStreamingType("mock_streaming_b");
    config.getTypeConfig().setPropertyValue("Host", "remote");
    clientNewer.addDevice("daqmirrored://device", config);

    const AddDeviceConfigPtr uploaded = serverNewer->uploadedConfig;
    expectNewerServerConfig(uploaded);

    const auto setup = clientDevice.saveConfiguration();
    clientContext = NullContext();
    connectClient();
    clientDevice.loadConfiguration(setup);
    expectNewerServerSavedValues(serverNewer->loadedConfig);
}

TEST_F(ConfigProtocolIntegrationTest, AddDeviceConfigOfNewerServerFailsTypedAccessorsOfRemovedField)
{
    addNewerServerDevice();
    const auto config = getClientNewerDevice().createAddDeviceConfig(createMirroredDeviceInfo(), "daqmirrored");

    ASSERT_THROW(config.getPrimaryAddressType(), NotFoundException);
}

TEST_F(ConfigProtocolIntegrationTest, BeginEndUpdateNestedPropertyObjectOrder)
{
    // This test reproduces a bug where SetPropertyValue("PropertyObject.Property") 
    // is called AFTER PropertyObject.EndUpdate but INSIDE Component.EndUpdate
    // Expected order:
    // 1. Component.beginUpdate() -> PropertyObject.beginUpdate() (recursive)
    // 2. PropertyObject.setPropertyValue("Property") (batched)
    // 3. Component.endUpdate() -> PropertyObject.endUpdate() -> SetPropertyValue("PropertyObject.Property")
    // 4. PropertyObject.EndUpdate event
    // 5. Component.EndUpdate event
    
    const PropertyObjectPtr serverMockChild = serverDevice.getPropertyValue("MockChild");
    const PropertyObjectPtr clientMockChild = clientDevice.getPropertyValue("MockChild");

    enum class State
    {
        init = 0,
        write = 1,
        nesteadObjEnded = 2,
        ComponentEnded = 3
    };
    
    State state = State::init;

    serverMockChild.getOnPropertyValueWrite("NestedStringProperty") += [&state](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
    {
        ASSERT_EQ(state, State::init) << "SetPropertyValue should be called BEFORE PropertyObject.EndUpdate";
        ASSERT_TRUE(args.getIsUpdating());
        state = State::write;
    };

    serverMockChild.getOnEndUpdate() += [&state] (PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_EQ(state, State::write) << "SetPropertyValue should be called BEFORE PropertyObject.EndUpdate";
        const auto propsChanged = args.getProperties();
        ASSERT_THAT(propsChanged, ElementsAre("NestedStringProperty"));
        state = State::nesteadObjEnded;
    };

    serverDevice.getOnEndUpdate() += [&state](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_EQ(state, State::nesteadObjEnded) << "Component.EndUpdate should be called AFTER PropertyObject.EndUpdate";
        state = State::ComponentEnded;
    };

    clientDevice.beginUpdate();
    clientMockChild.setPropertyValue("NestedStringProperty", "NewValue");
    clientDevice.endUpdate();

    ASSERT_EQ(state, State::ComponentEnded);
    ASSERT_EQ(clientMockChild.getPropertyValue("NestedStringProperty"), "NewValue");
    ASSERT_EQ(serverMockChild.getPropertyValue("NestedStringProperty"), "NewValue");
}

TEST_F(ConfigProtocolIntegrationTest, GetServers)
{
    ASSERT_GT(serverDevice.getServers().getCount(), 0u);

    ListPtr<IServer> servers;
    ASSERT_NO_THROW(servers = clientDevice.getServers());
    ASSERT_EQ(servers.getCount(), serverDevice.getServers().getCount());
}

TEST_F(ConfigProtocolIntegrationTest, ComponentTypesCarriedWithoutDefaultConfig)
{
    const auto serverSubDevice = serverDevice.getDevices()[0];
    const auto clientSubDevice = clientDevice.getDevices()[0];

    // The physical mock device is the one whose info carries a device type.
    const auto serverDeviceType = serverDevice.getDevices(search::LocalId("mock_phys_dev"))[0].getInfo().getDeviceType();
    const auto clientDeviceType = clientDevice.getDevices(search::LocalId("mock_phys_dev"))[0].getInfo().getDeviceType();
    ASSERT_TRUE(serverDeviceType.assigned());
    ASSERT_EQ(clientDeviceType.getId(), serverDeviceType.getId());
    ASSERT_EQ(clientDeviceType.getName(), serverDeviceType.getName());
    ASSERT_EQ(clientDeviceType.getConnectionStringPrefix(), serverDeviceType.getConnectionStringPrefix());

    const auto serverFbType = serverSubDevice.getFunctionBlocks()[0].getFunctionBlockType();
    const auto clientFbType = clientSubDevice.getFunctionBlocks()[0].getFunctionBlockType();
    ASSERT_EQ(clientFbType.getId(), serverFbType.getId());
    ASSERT_EQ(clientFbType.getName(), serverFbType.getName());
    ASSERT_EQ(clientFbType.getDescription(), serverFbType.getDescription());
    ASSERT_EQ(clientFbType.createDefaultConfig().getAllProperties().getCount(), 0u);

    const auto serverSrvType = serverDevice.getServers()[0].getServerType();
    const auto clientSrvType = clientDevice.getServers()[0].getServerType();
    ASSERT_EQ(clientSrvType.getName(), serverSrvType.getName());
    ASSERT_EQ(clientSrvType.getServerKind(), serverSrvType.getServerKind());

    // The carried module info is kept; the serialized module version does not replace it with a placeholder.
    serverFbType.asPtr<IComponentTypePrivate>()->setModuleInfo(ModuleInfo(VersionInfo(1, 2, 3), "ServerModule", "server_module"));
    auto secondClient = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
        NullContext(),
        [this](const PacketBuffer& requestPacket) { return server->processRequestAndGetReply(requestPacket); },
        [](const PacketBuffer& /*requestPacket*/) {},
        nullptr,
        nullptr,
        nullptr);
    const auto secondClientFbType = secondClient->connect().getDevices()[0].getFunctionBlocks()[0].getFunctionBlockType();
    ASSERT_EQ(secondClientFbType.getModuleInfo().getName(), "ServerModule");
    ASSERT_EQ(secondClientFbType.getModuleInfo().getId(), "server_module");
    ASSERT_EQ(secondClientFbType.getModuleInfo().getVersionInfo().getMinor(), 2u);
}

TEST_F(ConfigProtocolIntegrationTest, ServerKind)
{
    const auto serverSrv = serverDevice.getServers()[0];
    const auto clientSrv = clientDevice.getServers()[0];

    ASSERT_EQ(serverSrv.getServerType().getServerKind(), ServerKind::Adapter);
    ASSERT_EQ(clientSrv.getServerType().getServerKind(), ServerKind::Adapter);
    ASSERT_EQ(clientSrv.getServerType().getId(), serverSrv.getServerType().getId());
}

TEST_F(ConfigProtocolIntegrationTest, ServerGetAvailableFunctionBlockTypes)
{
    const auto serverSrv = serverDevice.getServers()[0];
    const auto clientSrv = clientDevice.getServers()[0];

    ASSERT_EQ(clientSrv.getAvailableFunctionBlockTypes(), serverSrv.getAvailableFunctionBlockTypes());
}

TEST_F(ConfigProtocolIntegrationTest, ServerAddAndRemoveFunctionBlock)
{
    const auto serverSrv = serverDevice.getServers()[0];
    const auto clientSrv = clientDevice.getServers()[0];

    const auto fb = clientSrv.addFunctionBlock("mockfb1");
    ASSERT_EQ(serverSrv.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(clientSrv.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(fb, clientSrv.getFunctionBlocks()[0]);
    ASSERT_EQ(fb.asPtr<IConfigClientObject>().getRemoteGlobalId(), serverSrv.getFunctionBlocks()[0].getGlobalId());
    ASSERT_EQ(fb.getSignals()[0].getDomainSignal(), fb.getSignals()[2]);

    clientSrv.removeFunctionBlock(fb);
    ASSERT_TRUE(fb.isRemoved());
    ASSERT_EQ(serverSrv.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(clientSrv.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, ServerFunctionBlocksNested)
{
    const auto serverSrv = serverDevice.getServers()[0];
    const auto clientSrv = clientDevice.getServers()[0];
    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    const auto fb = clientSrv.addFunctionBlock("mockfb1");
    const auto nestedFb = fb.addFunctionBlock("mockfb1", config);

    const auto serverFb = serverSrv.getFunctionBlocks()[0];
    ASSERT_EQ(serverFb.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(nestedFb.asPtr<IConfigClientObject>().getRemoteGlobalId(), serverFb.getFunctionBlocks()[0].getGlobalId());
    ASSERT_EQ(clientSrv.getFunctionBlocks(search::Recursive(search::Any())).getCount(), 2u);

    fb.removeFunctionBlock(nestedFb);
    ASSERT_TRUE(nestedFb.isRemoved());
    ASSERT_EQ(serverFb.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(fb.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, ServerFunctionBlockAddedOnServerSide)
{
    const auto serverSrv = serverDevice.getServers()[0];
    const auto clientSrv = clientDevice.getServers()[0];

    serverSrv.addFunctionBlock("mockfb1");
    ASSERT_EQ(clientSrv.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(clientSrv.getFunctionBlocks()[0].asPtr<IConfigClientObject>().getRemoteGlobalId(),
              serverSrv.getFunctionBlocks()[0].getGlobalId());

    serverSrv.removeFunctionBlock(serverSrv.getFunctionBlocks()[0]);
    ASSERT_EQ(clientSrv.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ConfigProtocolIntegrationTest, ServerFunctionBlocksNeedProtocolVersion26)
{
    auto oldClient = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
        NullContext(),
        [this](const PacketBuffer& requestPacket) { return server->processRequestAndGetReply(requestPacket); },
        [](const PacketBuffer& /*requestPacket*/) {},
        nullptr,
        nullptr,
        nullptr);
    const auto oldClientDevice = oldClient->connect(nullptr, 25);
    const auto oldClientSrv = oldClientDevice.getServers()[0];

    ASSERT_EQ(oldClientSrv.getAvailableFunctionBlockTypes().getCount(), 0u);
    ASSERT_THROW(oldClientSrv.addFunctionBlock("mockfb1"), ServerVersionTooLowException);
    ASSERT_EQ(serverDevice.getServers()[0].getFunctionBlocks().getCount(), 0u);
}
