#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/device_ptr.h>
#include <opendaq/gmock/device.h>
#include <opendaq/gmock/function_block.h>
#include <opendaq/gmock/component.h>
#include <opendaq/gmock/input_port.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/component_holder_ptr.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;

class MockComponentFinder: public IComponentFinder
{
public:
    MOCK_METHOD(ComponentPtr, findComponent, (const std::string&), (override));
};

class ConfigProtocolTest : public Test
{
public:
    ConfigProtocolTest()
        : Test()
    {
    }

    void SetUp() override
    {
        const auto anonymousUser = User("", "");
        const auto permissions = PermissionsBuilder().inherit(true).assign("everyone", PermissionMaskBuilder().read().write().execute()).build();
        device->getPermissionManager().setPermissions(permissions);

        EXPECT_CALL(device.mock(), getContext(_)).WillRepeatedly(Get(NullContext()));
        server = std::make_unique<ConfigProtocolServer>(device, std::bind(&ConfigProtocolTest::serverNotificationReady, this, std::placeholders::_1), anonymousUser);
        client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                NullContext(),
                std::bind(&ConfigProtocolTest::sendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigProtocolTest::sendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                std::bind(&ConfigProtocolTest::onServerNotificationReceived, this, std::placeholders::_1)
            );
        std::unique_ptr<IComponentFinder> m = std::make_unique<MockComponentFinder>();
        server->setComponentFinder(m);
    }

protected:
    MockDevice::Strict device;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    BaseObjectPtr notificationObj;

    virtual PacketBuffer getRequestReplyFromServer(const PacketBuffer& requestPacket) const
    {
        return server->processRequestAndGetReply(requestPacket);
    }

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket)
    {
        client->triggerNotificationPacket(notificationPacket);
    }

    // client handling
    PacketBuffer sendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = getRequestReplyFromServer(requestPacket);
        return replyPacket;
    }

    void sendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        server->processNoReplyRequest(requestPacket);
    }

    bool onServerNotificationReceived(const BaseObjectPtr& obj)
    {
        notificationObj = obj;
        return false;
    }

    MockComponentFinder& getMockComponentFinder()
    {
        const auto& componentFinder = server->getComponentFinder();
        return *dynamic_cast<MockComponentFinder*>(componentFinder.get());
    }
};

TEST_F(ConfigProtocolTest, Connect)
{
    EXPECT_CALL(device.mock(), getLocalId(_)).WillOnce(Get(String("Id")));
    EXPECT_CALL(device.mock(), getParent(_)).WillRepeatedly(Get(Component(NullContext(), nullptr, "parent")));
    ASSERT_THROW(client->connect(), ConfigProtocolException);
}

TEST_F(ConfigProtocolTest, ServerNotification)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("key", "value");
    server->sendNotification(dict);
    ASSERT_EQ(notificationObj, dict);
}

TEST_F(ConfigProtocolTest, SetPropertyValueComponentNotFound)
{
    EXPECT_CALL(getMockComponentFinder(), findComponent(_)).WillOnce(Return(nullptr));
    ASSERT_THROW(client->getClientComm()->setPropertyValue("/dev/comp/test", "PropName", "PropValue"), NotFoundException);
}

TEST_F(ConfigProtocolTest, SetPropertyValueComponent)
{
    MockComponent::Strict component;
    component->addProperty(StringPropertyBuilder("PropName", "-").build());
    component->getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(device->getPermissionManager());

    EXPECT_CALL(getMockComponentFinder(), findComponent(_)).WillOnce(Return(component));
    EXPECT_CALL(component.mock(), getParent(_)).WillOnce(Get<ComponentPtr>(nullptr));

    client->getClientComm()->setPropertyValue("/dev/comp/test", "PropName", "PropValue");

    ASSERT_EQ(component->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, SetPropertyValueRoot)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillOnce(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    device->addProperty(StringPropertyBuilder("PropName", "-").build());

    client->getClientComm()->setPropertyValue("//root", "PropName", "PropValue");

    ASSERT_EQ(device->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, SetProtectedPropertyValueRoot)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillRepeatedly(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    device->addProperty(StringPropertyBuilder("PropName", "-").setReadOnly(True).build());

    ASSERT_THROW(client->getClientComm()->setPropertyValue("//root", "PropName", "PropValue"), AccessDeniedException);
    client->getClientComm()->setProtectedPropertyValue("//root", "PropName", "PropValue");

    ASSERT_EQ(device->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, ClearPropertyValueRoot)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillRepeatedly(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    device->addProperty(StringPropertyBuilder("PropName", "-").build());
    device->setPropertyValue("PropName", "PropValue");

    client->getClientComm()->clearPropertyValue("//root", "PropName");

    ASSERT_EQ(device->getPropertyValue("PropName"), "-");
}

TEST_F(ConfigProtocolTest, GetPropertyValueRoot)
{
    device->addProperty(StringPropertyBuilder("PropName", "-").build());
    device->setPropertyValue("PropName", "val");

    const auto value = client->getClientComm()->getPropertyValue("//root", "PropName");

    ASSERT_EQ(value, "val");
}

TEST_F(ConfigProtocolTest, GetObjectPropertyValue)
{
    const auto defaultValue = PropertyObject();
    defaultValue.addProperty(StringPropertyBuilder("StringProp", "-").build());
    defaultValue.addProperty(IntPropertyBuilder("IntProp", 0).build());

    device->addProperty(ObjectPropertyBuilder("PropName", defaultValue).build());

    const auto clientVal = client->getClientComm()->getPropertyValue("//root", "PropName").asPtr<IPropertyObject>();

    ASSERT_EQ(clientVal.getPropertyValue("StringProp"), "-");
    ASSERT_EQ(clientVal.getPropertyValue("IntProp"), 0);
}

TEST_F(ConfigProtocolTest, GetChildObjectPropertyValue)
{
    const auto defaultValue = PropertyObject();
    defaultValue.addProperty(StringPropertyBuilder("StringProp", "-").build());
    defaultValue.addProperty(IntPropertyBuilder("IntProp", 0).build());

    device->addProperty(ObjectPropertyBuilder("PropName", defaultValue).build());

    const auto s = client->getClientComm()->getPropertyValue("//root", "PropName.StringProp");
    ASSERT_EQ(s, "-");

    const auto i = client->getClientComm()->getPropertyValue("//root", "PropName.IntProp");
    ASSERT_EQ(i, 0);
}

TEST_F(ConfigProtocolTest, SetChildObjectPropertyValue)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillOnce(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    const auto defaultValue = PropertyObject();
    defaultValue.addProperty(StringPropertyBuilder("StringProp", "-").build());
    defaultValue.addProperty(IntPropertyBuilder("IntProp", 0).build());

    device->addProperty(ObjectPropertyBuilder("PropName", defaultValue).build());

    client->getClientComm()->setPropertyValue("//root", "PropName.StringProp", "val");

    ASSERT_EQ(device->getPropertyValue("PropName.StringProp"), "val");
}

TEST_F(ConfigProtocolTest, CallProcedurePropertyOneParam)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillOnce(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    Int p1 = 0;
    device->addProperty(
        FunctionPropertyBuilder(
            "Func", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("P1", CoreType::ctInt))))
            .setReadOnly(True)
            .build());
    device->asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Func",
                                                                        Procedure(
                                                                            [&p1](Int _p1)
                                                                            {
                                                                                p1 = _p1;
                                                                            }));

    client->getClientComm()->callProperty("//root", "Func", 2);

    ASSERT_EQ(p1, 2);
}

TEST_F(ConfigProtocolTest, CallProcedurePropertyTwoParams)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillOnce(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    Int p1 = 0;
    StringPtr p2;
    device->addProperty(
        FunctionPropertyBuilder(
            "Func", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("P1", CoreType::ctInt), ArgumentInfo("P2", CoreType::ctString))))
            .setReadOnly(True)
            .build());
    device->asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Func", Procedure([&p1, &p2](Int _p1, const StringPtr& _p2)
    {
        p1 = _p1;
        p2 = _p2;
    }));

    const auto params = List<IBaseObject>(2, "value");
    client->getClientComm()->callProperty("//root", "Func", params);

    ASSERT_EQ(p1, 2);
    ASSERT_EQ(p2, "value");
}

TEST_F(ConfigProtocolTest, GetAvailableDeviceTypes)
{
    const auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(StringPropertyBuilder("Prop", "value").build());
    defaultConfig.getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(device->getPermissionManager());

    auto fbTypes = Dict<IString, IFunctionBlockType>();
    fbTypes.set("Id", FunctionBlockType("Id", "Name", "Desc", defaultConfig));

    EXPECT_CALL(device.mock(), getAvailableFunctionBlockTypes(_)).WillOnce(daq::Get<DictPtr<IString, IFunctionBlockType>>(fbTypes));

    const DictPtr<IString, IFunctionBlockType> value = client->getClientComm()->sendComponentCommand("//root", "GetAvailableFunctionBlockTypes");
    ASSERT_EQ(fbTypes.get("Id"), value.get("Id"));
    ASSERT_EQ(fbTypes.get("Id").createDefaultConfig().getPropertyValue("Prop"), "value");
}

TEST_F(ConfigProtocolTest, GetDeviceInfo)
{
    const auto devInfo = DeviceInfo("connectionString", "Name");
    devInfo.getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(device->getPermissionManager());

    StringPtr fbId;
    EXPECT_CALL(device.mock(), getInfo(_)).WillOnce(daq::Get<DeviceInfoPtr>(devInfo));

    const DeviceInfoPtr newDevInfo = client->getClientComm()->sendComponentCommand("//root", "GetInfo");
    ASSERT_EQ(newDevInfo.getConnectionString(), devInfo.getConnectionString());
    ASSERT_EQ(newDevInfo.getName(), devInfo.getName());
}

TEST_F(ConfigProtocolTest, AddFunctionBlock)
{
    StringPtr fbId;
    EXPECT_CALL(device.mock(), addFunctionBlock(_, _, _))
        .WillOnce([&fbId](IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config) -> ErrCode
            {
                fbId = typeId;
                return createObject<IFunctionBlock, FunctionBlock>(functionBlock,
                                                                   FunctionBlockType("fbId", "FunctionBlock", "Function Block"),
                                                                   NullContext(),
                                                                   nullptr,
                                                                   "fb");
            });

    EXPECT_CALL(device.mock(), isLocked(_))
        .WillOnce(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    auto params = Dict<IString, IBaseObject>({{"TypeId", "fbId"}});
    const ComponentHolderPtr fbHolder = client->getClientComm()->sendComponentCommand("//root", "AddFunctionBlock", params);
    ASSERT_EQ(fbHolder.getLocalId(), "fb");
    const FunctionBlockPtr fb = fbHolder.getComponent();

    ASSERT_EQ(fbId, "fbId");
}

TEST_F(ConfigProtocolTest, RemoveFunctionBlock)
{
    MockFunctionBlock::Strict fb;
    auto functionBlocks = List<IFunctionBlock>(fb.ptr);

    EXPECT_CALL(fb.mock(), getLocalId(_)).WillRepeatedly(Get<StringPtr>(String("lid")));

    EXPECT_CALL(device.mock(), isLocked(_))
        .WillRepeatedly(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    EXPECT_CALL(device.mock(), getFunctionBlocks(_, _))
        .WillRepeatedly([functionBlocks](IList** fbs, ISearchFilter* searchFilter)
            {
                auto outFbs = List<IFunctionBlock>();
                const auto sf = SearchFilterPtr::Borrow(searchFilter);
                for (const auto fb : functionBlocks)
                    if (sf.acceptsComponent(fb))
                        outFbs.pushBack(fb);
                *fbs = outFbs.detach();
                return OPENDAQ_SUCCESS;
            });


    EXPECT_CALL(device.mock(), removeFunctionBlock(_))
        .WillRepeatedly(
            [&functionBlocks](IFunctionBlock* functionBlock) -> ErrCode
            {
                const auto fb = FunctionBlockPtr::Borrow(functionBlock);
                if (fb == functionBlocks[0])
                    return OPENDAQ_SUCCESS;

                return OPENDAQ_NOTFOUND;
            });

    auto params = Dict<IString, IBaseObject>({{"LocalId", "lid"}});
    ASSERT_NO_THROW(client->getClientComm()->sendComponentCommand("//root", "RemoveFunctionBlock", params));

    params = Dict<IString, IBaseObject>({{"LocalId", "invalid"}});
    ASSERT_THROW(client->getClientComm()->sendComponentCommand("//root", "RemoveFunctionBlock", params), NotFoundException);
}

TEST_F(ConfigProtocolTest, ConnectSignalToInputPort)
{
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    inputPort->getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(device->getPermissionManager());
    signal->getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(device->getPermissionManager());

    EXPECT_CALL(getMockComponentFinder(), findComponent(_))
        .WillOnce(Return(inputPort.ptr.asPtr<IComponent>()))
        .WillOnce(Return(signal.ptr.asPtr<IComponent>()));
    EXPECT_CALL(inputPort.mock(), connect(_)).WillOnce(Return(OPENDAQ_SUCCESS));
    EXPECT_CALL(inputPort.mock(), getParent(_)).WillOnce(Get<ComponentPtr>(nullptr));

    auto params = ParamsDict({{"SignalId", "sig"}});
    client->getClientComm()->sendComponentCommand("/dev/comp/test", "ConnectSignal", params);
}

TEST_F(ConfigProtocolTest, DisconnectSignalFromInputPort)
{
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    inputPort->getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(device->getPermissionManager());

    EXPECT_CALL(getMockComponentFinder(), findComponent(_)).WillOnce(Return(inputPort.ptr.asPtr<IComponent>()));
    EXPECT_CALL(inputPort.mock(), disconnect()).WillOnce(Return(OPENDAQ_SUCCESS));
    EXPECT_CALL(inputPort.mock(), getParent(_)).WillOnce(Get<ComponentPtr>(nullptr));

    client->getClientComm()->sendComponentCommand("/dev/comp/test", "DisconnectSignal");
}

TEST_F(ConfigProtocolTest, GetTypeManager)
{
    EXPECT_CALL(device.mock(), getContext(_)).WillRepeatedly(Get(NullContext()));
    const TypeManagerPtr typeManager = client->getClientComm()->sendCommand("GetTypeManager");
}

TEST_F(ConfigProtocolTest, BeginEndUpdate)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillRepeatedly(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    device->addProperty(StringPropertyBuilder("PropName", "-").build());
    ASSERT_EQ(device->getPropertyValue("PropName"), "-");

    client->getClientComm()->sendComponentCommand("//root", "BeginUpdate");
    client->getClientComm()->setPropertyValue("//root", "PropName", "val");
    ASSERT_EQ(device->getPropertyValue("PropName"), "-");
    client->getClientComm()->sendComponentCommand("//root", "EndUpdate");
    ASSERT_EQ(device->getPropertyValue("PropName"), "val");
}

TEST_F(ConfigProtocolTest, BeginEndUpdateWithProps)
{
    EXPECT_CALL(device.mock(), isLocked(_))
        .WillRepeatedly(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    device->addProperty(StringPropertyBuilder("PropName", "-").build());
    ASSERT_EQ(device->getPropertyValue("PropName"), "-");

    client->getClientComm()->sendComponentCommand("//root", "BeginUpdate");

    auto prop = Dict<IString, IBaseObject>({{"Name", "PropName"}, {"ProtectedAccess", False}, {"SetValue", True}, {"Value", "val"}});
    auto props = List<IDict>(prop);
    auto params = Dict<IString, IBaseObject>({{"Props", props}});

    ASSERT_THROW(client->getClientComm()->sendComponentCommand("//root", "EndUpdate", params), NotSupportedException);
    server->setProtocolVersion(1);
    client->getClientComm()->sendComponentCommand("//root", "EndUpdate", params);

    ASSERT_EQ(device->getPropertyValue("PropName"), "val");
}

TEST_F(ConfigProtocolTest, SetNameAndDescriptionAttribute)
{
    StringPtr deviceName;
    StringPtr deviceDescription;

    EXPECT_CALL(device.mock(), isLocked(_))
        .WillRepeatedly(
            [](daq::Bool* locked) -> ErrCode
            {
                *locked = false;
                return OPENDAQ_SUCCESS;
            });

    EXPECT_CALL(device.mock(), setName(_)).WillOnce([&](IString* name)
    {
        deviceName = name;
        return OPENDAQ_SUCCESS;
    });

    EXPECT_CALL(device.mock(), setDescription(_))
        .WillOnce(
            [&](IString* description)
            {
                deviceDescription = description;
                return OPENDAQ_SUCCESS;
            });

    client->getClientComm()->setAttributeValue("//root", "Name", "devName");
    ASSERT_EQ(deviceName, "devName");
    client->getClientComm()->setAttributeValue("//root", "Description", "devDescription");
    ASSERT_EQ(deviceDescription, "devDescription");
}

TEST_F(ConfigProtocolTest, InputPortAcceptsSignal)
{
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(getMockComponentFinder(), findComponent(_))
        .WillOnce(Return(inputPort.ptr.asPtr<IComponent>()))
        .WillOnce(Return(signal.ptr.asPtr<IComponent>()));
    EXPECT_CALL(inputPort.mock(), getParent(_)).WillRepeatedly(Get(Component(NullContext(), nullptr, "parent")));
    EXPECT_CALL(inputPort.mock(), acceptsSignal(_, _)).WillOnce(Return(OPENDAQ_SUCCESS));

    auto params = ParamsDict({{"SignalId", "sig"}});
    client->getClientComm()->sendComponentCommand("/dev/comp/test", "AcceptsSignal", params);
}

TEST_F(ConfigProtocolTest, DeviceGetAvailableDevices)
{
    MockDevice::Strict device;

    EXPECT_CALL(device.mock(), isLocked(_))
    .WillRepeatedly(
        [](daq::Bool* locked) -> ErrCode
        {
            *locked = false;
            return OPENDAQ_SUCCESS;
        });

    EXPECT_CALL(getMockComponentFinder(), findComponent(_))
        .WillOnce(Return(device.ptr.asPtr<IComponent>()));

    EXPECT_CALL(device.mock(), getAvailableDevices(_)).WillOnce(Return(OPENDAQ_SUCCESS));

    client->getClientComm()->sendComponentCommand("/dev", "GetAvailableDevices");
}


class RejectConnectionTest : public ConfigProtocolTest
{
public:
    RejectConnectionTest()
        : ConfigProtocolTest()
    {
    }

protected:
    PacketBuffer getRequestReplyFromServer(const PacketBuffer& requestPacket) const override
    {
        return ConfigProtocolServer::generateConnectionRejectedReply(
            requestPacket.getId(),
            OPENDAQ_ERR_GENERALERROR,
            "Test connection rejected",
            JsonSerializer()
        );
    }
};

TEST_F(RejectConnectionTest, Connect)
{
    ASSERT_THROW_MSG(client->connect(), GeneralErrorException, "Test connection rejected");
}
