#include <gtest/gtest.h>
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
#include <config_protocol/component_holder_ptr.h>
#include <config_protocol/config_client_device_impl.h>

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
        EXPECT_CALL(device.mock(), getContext(_)).WillRepeatedly(Get(NullContext()));
        server = std::make_unique<ConfigProtocolServer>(device, std::bind(&ConfigProtocolTest::serverNotificationReady, this, std::placeholders::_1));
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(NullContext(), std::bind(&ConfigProtocolTest::sendRequest, this, std::placeholders::_1), std::bind(&ConfigProtocolTest::onServerNotificationReceived, this, std::placeholders::_1));

        std::unique_ptr<IComponentFinder> m = std::make_unique<MockComponentFinder>();
        server->setComponentFinder(m);
    }

protected:
    MockDevice::Strict device;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    BaseObjectPtr notificationObj;

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket)
    {
        client->triggerNotificationPacket(notificationPacket);
    }

    // client handling
    PacketBuffer sendRequest(const PacketBuffer& requestPacket)
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
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
    EXPECT_CALL(device.mock(), getLocalId(_)).WillOnce(Get(String("id")));
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

    EXPECT_CALL(getMockComponentFinder(), findComponent(_)).WillOnce(Return(component));

    client->getClientComm()->setPropertyValue("/dev/comp/test", "PropName", "PropValue");

    ASSERT_EQ(component->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, SetPropertyValueRoot)
{
    device->addProperty(StringPropertyBuilder("PropName", "-").build());

    client->getClientComm()->setPropertyValue("//root", "PropName", "PropValue");

    ASSERT_EQ(device->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, SetProtectedPropertyValueRoot)
{
    device->addProperty(StringPropertyBuilder("PropName", "-").setReadOnly(True).build());

    ASSERT_THROW(client->getClientComm()->setPropertyValue("//root", "PropName", "PropValue"), AccessDeniedException);
    client->getClientComm()->setProtectedPropertyValue("//root", "PropName", "PropValue");

    ASSERT_EQ(device->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, ClearPropertyValueRoot)
{
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
    const auto defaultValue = PropertyObject();
    defaultValue.addProperty(StringPropertyBuilder("StringProp", "-").build());
    defaultValue.addProperty(IntPropertyBuilder("IntProp", 0).build());

    device->addProperty(ObjectPropertyBuilder("PropName", defaultValue).build());

    client->getClientComm()->setPropertyValue("//root", "PropName.StringProp", "val");

    ASSERT_EQ(device->getPropertyValue("PropName.StringProp"), "val");
}

TEST_F(ConfigProtocolTest, CallProcedurePropertyOneParam)
{
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
    defaultConfig.addProperty(StringPropertyBuilder("prop", "value").build());

    auto fbTypes = Dict<IString, IFunctionBlockType>();
    fbTypes.set("id", FunctionBlockType("id", "name", "desc", Function([&defaultConfig] {
            return defaultConfig;
        })));

    EXPECT_CALL(device.mock(), getAvailableFunctionBlockTypes(_)).WillOnce(daq::Get<DictPtr<IString, IFunctionBlockType>>(fbTypes));

    const DictPtr<IString, IFunctionBlockType> value = client->getClientComm()->sendComponentCommand("//root", "GetAvailableFunctionBlockTypes");
    ASSERT_EQ(fbTypes.get("id"), value.get("id"));
    ASSERT_EQ(fbTypes.get("id").createDefaultConfig().getPropertyValue("prop"), "value");
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

    auto params = Dict<IString, IBaseObject>({{"TypeId", "fbId"}});
    const ComponentHolderPtr fbHolder = client->getClientComm()->sendComponentCommand("//root", "AddFunctionBlock", params);
    ASSERT_EQ(fbHolder.getLocalId(), "fb");
    const FunctionBlockPtr fb = fbHolder.getComponent();

    ASSERT_EQ(fbId, "fbId");
}

TEST_F(ConfigProtocolTest, ConnectSignalToInputPort)
{
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(getMockComponentFinder(), findComponent(_))
        .WillOnce(Return(inputPort.ptr.asPtr<IComponent>()))
        .WillOnce(Return(signal.ptr.asPtr<IComponent>()));
    EXPECT_CALL(inputPort.mock(), connect(_)).WillOnce(Return(OPENDAQ_SUCCESS));

    auto params = ParamsDict({{"SignalId", "sig"}});
    client->getClientComm()->sendComponentCommand("/dev/comp/test", "ConnectSignal", params);
}

TEST_F(ConfigProtocolTest, GetTypeManager)
{
    EXPECT_CALL(device.mock(), getContext(_)).WillRepeatedly(Get(NullContext()));
    const TypeManagerPtr typeManager = client->getClientComm()->sendCommand("GetTypeManager");
}
