// ReSharper disable CppClangTidyModernizeAvoidBind
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/context_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <config_protocol/config_client_object_ptr.h>
#include <config_protocol/config_client_device_impl.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;
using namespace std::placeholders;

class MockFb1Impl final : public FunctionBlock
{
public:
    MockFb1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : FunctionBlock(FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId)
    {
        createAndAddSignal("sig1");
        createAndAddSignal("sig2");
        createAndAddInputPort("ip", PacketReadyNotification::None);
    }
};

class MockFb2Impl final : public FunctionBlock
{
public:
    MockFb2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : FunctionBlock(FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId)
    {
        createAndAddSignal("sig");
        createAndAddInputPort("ip", PacketReadyNotification::None);
        const auto childFb = createWithImplementation<IFunctionBlock, MockFb1Impl>(ctx, this->functionBlocks, "childFb");
        addNestedFunctionBlock(childFb);
    }
};

class MockChannel1Impl final : public Channel
{
public:
    MockChannel1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Channel(FunctionBlockType("ch", "", ""), ctx, parent, localId)
    {
        createAndAddSignal("sig_ch");
        const auto childFb = createWithImplementation<IFunctionBlock, MockFb1Impl>(ctx, this->functionBlocks, "childFb");
        addNestedFunctionBlock(childFb);
    }
};

class MockChannel2Impl final : public Channel
{
public:
    MockChannel2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Channel(FunctionBlockType("ch", "", ""), ctx, parent, localId)
    {
        createAndAddSignal("sig_ch");
        createAndAddInputPort("ip", PacketReadyNotification::None);

        objPtr.addProperty(StringPropertyBuilder("StrProp", "-").build());
        objPtr.addProperty(StringPropertyBuilder("StrPropProtected", "").setReadOnly(True).build());
    }
};

class MockDevice1Impl final : public Device
{
public:
    MockDevice1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
        createAndAddSignal("sig_device");

        auto aiIoFolder = this->addIoFolder("ai", ioFolder);
        createAndAddChannel<MockChannel1Impl>(aiIoFolder, "ch");

        const auto fb = createWithImplementation<IFunctionBlock, MockFb1Impl>(ctx, this->functionBlocks, "fb");
        addNestedFunctionBlock(fb);
    }

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        auto fbTypes = Dict<IString, IFunctionBlockType>({{"mockfb1", FunctionBlockType("mockfb1", "MockFB1", "Mock FB1", nullptr)}});
        return fbTypes;
    }

    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override
    {
        if (typeId == "mockfb1")
        {
            if (!config.assigned())
                throw InvalidParameterException();

            const StringPtr param = config.getPropertyValue("Param");
            if (param != "Value")
                throw InvalidParameterException();

            const auto fb = createWithImplementation<IFunctionBlock, MockFb1Impl>(context, this->functionBlocks, "newFb");
            addNestedFunctionBlock(fb);
            return fb;
        }
        throw NotFoundException();
    }
};

class MockDevice2Impl final : public Device
{
public:
    MockDevice2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
        createAndAddSignal("sig_device");

        auto aiIoFolder = this->addIoFolder("ai", ioFolder);
        createAndAddChannel<MockChannel2Impl>(aiIoFolder, "ch");

        const auto dev = createWithImplementation<IDevice, MockDevice1Impl>(ctx, this->devices, "dev");
        devices.addItem(dev);

        const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"float", 5.123}});
        const auto defStructValue = Struct("FooStruct", structMembers, manager.getRef());

        objPtr.addProperty(StructPropertyBuilder("StructProp", defStructValue).build());
    }
};

class ConfigProtocolIntegrationTest : public Test
{
public:
    static StringPtr serializeComponent(const ComponentPtr& component)
    {
        const auto serializer = JsonSerializer(True);
        component.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }

    static DevicePtr createServerDevice()
    {
        const auto serverDevice = createWithImplementation<IDevice, MockDevice2Impl>(NullContext(), nullptr, "root_dev");
        return serverDevice;
    }

    static PacketBuffer sendPacket(ConfigProtocolServer& server, const PacketBuffer& requestPacket)
    {
        auto replyPacket = server.processRequestAndGetReply(requestPacket);
        return replyPacket;
    }
};

TEST_F(ConfigProtocolIntegrationTest, Connect)
{
    const auto serverDevice = createServerDevice();
    const auto serverDeviceSerialized = serializeComponent(serverDevice);

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();
    const auto clientDeviceSerialized = serializeComponent(clientDevice);
    ASSERT_EQ(serverDeviceSerialized, clientDeviceSerialized);
}

TEST_F(ConfigProtocolIntegrationTest, RemoteGlobalIds)
{
    const auto serverDevice = createServerDevice();
    const auto serverDeviceSerialized = serializeComponent(serverDevice);

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

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
    const auto serverDevice = createServerDevice();
    const auto serverDeviceSerialized = serializeComponent(serverDevice);

    ConfigProtocolServer server(serverDevice, nullptr);

    const auto clientContext = NullContext();
    ConfigProtocolClient<ConfigClientDeviceImpl> client(clientContext, std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto parentComponent = Component(clientContext, nullptr, "cmp");

    const auto clientDevice = client.connect(parentComponent);

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
    const auto serverDevice = createServerDevice();
    const auto serverDeviceSerialized = serializeComponent(serverDevice);

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();
    checkComponentForConfigClientObject(clientDevice);
}

TEST_F(ConfigProtocolIntegrationTest, GetInitialPropertyValue)
{
    const auto serverDevice = createServerDevice();
    serverDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");

    const auto serverDeviceSerialized = serializeComponent(serverDevice);

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));

    const auto clientDeviceSerialized = serializeComponent(clientDevice);
    ASSERT_EQ(serverDeviceSerialized, clientDeviceSerialized);
}


TEST_F(ConfigProtocolIntegrationTest, SetPropertyValue)
{
    const auto serverDevice = createServerDevice();
    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");

    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));
}

TEST_F(ConfigProtocolIntegrationTest, SetProtectedPropertyValue)
{
    const auto serverDevice = createServerDevice();
    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    ASSERT_THROW(clientDevice.getChannels()[0].setPropertyValue("StrPropProtected", "SomeValue"), AccessDeniedException);

    clientDevice.getChannels()[0].asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrPropProtected", "SomeValue");

    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrPropProtected"), "SomeValue");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrPropProtected"), clientDevice.getChannels()[0].getPropertyValue("StrPropProtected"));
}

TEST_F(ConfigProtocolIntegrationTest, ClearPropertyValue)
{
    const auto serverDevice = createServerDevice();
    serverDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    clientDevice.getChannels()[0].clearPropertyValue("StrProp");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getChannels()[0].getPropertyValue("StrProp"), clientDevice.getChannels()[0].getPropertyValue("StrProp"));
}

TEST_F(ConfigProtocolIntegrationTest, CallFuncProp)
{
    const auto serverDevice = createServerDevice();
    const auto serverCh = serverDevice.getChannels()[0];
    const auto funcProp =
        FunctionPropertyBuilder("FuncProp", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt))))
            .setReadOnly(True)
            .build();
    serverCh.addProperty(funcProp);

    serverCh.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("FuncProp", Function([](Int a, Int b) { return a + b; }));

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    const auto clientFuncPropValue = clientDevice.getChannels()[0].getPropertyValue("FuncProp");

    Int r = clientFuncPropValue.call(2, 4);
    ASSERT_EQ(r, 6);
}

TEST_F(ConfigProtocolIntegrationTest, CallProcProp)
{
    const auto serverDevice = createServerDevice();
    const auto serverCh = serverDevice.getChannels()[0];
    const auto procProp =
        FunctionPropertyBuilder("ProcProp", ProcedureInfo())
            .setReadOnly(True)
            .build();
    serverCh.addProperty(procProp);

    bool called = false;
    serverCh.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("ProcProp", Procedure([&called] { called = true; }));

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    const auto clientProcPropValue = clientDevice.getChannels()[0].getPropertyValue("ProcProp");

    clientProcPropValue.dispatch();
    ASSERT_TRUE(called);
}

TEST_F(ConfigProtocolIntegrationTest, GetAvailableFunctionBlockTypes)
{
    const auto serverDevice = createServerDevice();

    ConfigProtocolServer server(serverDevice, nullptr);
    const auto serverSubDevice = serverDevice.getDevices()[0];

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();
    const auto clientSubDevice = clientDevice.getDevices()[0];

    const auto fbTypes = clientSubDevice.getAvailableFunctionBlockTypes();
    ASSERT_EQ(fbTypes, serverSubDevice.getAvailableFunctionBlockTypes());
}

TEST_F(ConfigProtocolIntegrationTest, AddFunctionBlockNotFound)
{
    const auto serverDevice = createServerDevice();

    ConfigProtocolServer server(serverDevice, nullptr);
    const auto serverSubDevice = serverDevice.getDevices()[0];

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();
    const auto clientSubDevice = clientDevice.getDevices()[0];

    ASSERT_THROW(clientSubDevice.addFunctionBlock("someFb"), NotFoundException);
}

TEST_F(ConfigProtocolIntegrationTest, AddFunctionBlock)
{
    const auto serverDevice = createServerDevice();

    ConfigProtocolServer server(serverDevice, nullptr);
    const auto serverSubDevice = serverDevice.getDevices()[0];

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();
    const auto clientSubDevice = clientDevice.getDevices()[0];

    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);

    ASSERT_EQ(fb, clientSubDevice.getFunctionBlocks()[1]);

    ASSERT_EQ(fb.asPtr<IConfigClientObject>().getRemoteGlobalId(), serverDevice.getDevices()[0].getFunctionBlocks()[1].getGlobalId());
}

TEST_F(ConfigProtocolIntegrationTest, GetInitialStructPropertyValue)
{
    const auto serverDevice = createServerDevice();
    const auto serverDeviceSerialized = serializeComponent(serverDevice);

    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    ASSERT_EQ(serverDevice.getPropertyValue("StructProp"), clientDevice.getPropertyValue("StructProp"));
}

TEST_F(ConfigProtocolIntegrationTest, SetStructPropertyValue)
{
    const auto serverDevice = createServerDevice();
    ConfigProtocolServer server(serverDevice, nullptr);

    ConfigProtocolClient<ConfigClientDeviceImpl> client(NullContext(), std::bind(sendPacket, std::ref(server), _1), nullptr);

    const auto clientDevice = client.connect();

    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar1"}, {"integer", 11}, {"float", 5.223}});
    const auto structVal = Struct("FooStruct", structMembers, serverDevice.getContext().getTypeManager());
    serverDevice.setPropertyValue("StructProp", structVal);

    ASSERT_EQ(serverDevice.getPropertyValue("StructProp"), structVal);
    ASSERT_EQ(serverDevice.getPropertyValue("StructProp"), clientDevice.getPropertyValue("StructProp"));
}
