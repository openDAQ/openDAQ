// ReSharper disable CppClangTidyModernizeAvoidBind
#include <opendaq/component.h>
#include <coreobjects/core_event_args.h>
#include <config_protocol/config_client_object_ptr.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include "test_utils.h"
#include "coreobjects/argument_info_factory.h"
#include "coreobjects/callable_info_factory.h"
#include "opendaq/context_factory.h"
#include <config_protocol/config_client_device_impl.h>

#include "opendaq/packet_factory.h"

using namespace daq;
using namespace config_protocol;
using namespace testing;
using namespace std::placeholders;

class ConfigProtocolIntegrationTest : public Test
{
public:
    void SetUp() override
    {
        serverDevice = test_utils::createServerDevice();
        serverDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(serverDevice, std::bind(&ConfigProtocolIntegrationTest::serverNotificationReady, this, std::placeholders::_1));

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(clientContext, std::bind(&ConfigProtocolIntegrationTest::sendRequest, this, std::placeholders::_1), nullptr);

        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    static StringPtr serializeComponent(const ComponentPtr& component)
    {
        const auto serializer = JsonSerializer(True);
        component.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }
    
    PacketBuffer sendRequest(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
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

TEST_F(ConfigProtocolIntegrationTest, InputPortConnected)
{
    ASSERT_EQ(serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
              serverDevice.getDevices()[0].getSignals()[0]);

    ASSERT_EQ(clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].getSignal(),
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
    
    ConfigProtocolClient<ConfigClientDeviceImpl> clientNew(clientContext, std::bind(&ConfigProtocolIntegrationTest::sendRequest, this, std::placeholders::_1), nullptr);

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

TEST_F(ConfigProtocolIntegrationTest, SetPropertyValue)
{
    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");

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
    ASSERT_EQ(clientSubDevice.getDomain().getUnit(), Unit("s", -1, "second", "time"));
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
    ASSERT_EQ(serverDevice.getDevices()[0].getChannels()[0].getSignals()[0].getDomainSignal(),
              serverDevice.getDevices()[0].getChannels()[0].getSignals()[1]);

    ASSERT_EQ(clientDevice.getDevices()[0].getChannels()[0].getSignals()[0].getDomainSignal(),
              clientDevice.getDevices()[0].getChannels()[0].getSignals()[1]);
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

TEST_F(ConfigProtocolIntegrationTest, DeviceInfoChanges)
{
    const auto serverDeviceInfo = serverDevice.getInfo();
    const auto clientDeviceInfo = clientDevice.getInfo();

    ASSERT_EQ(serverDeviceInfo.getName(), clientDeviceInfo.getName());

    const auto info1 = serverDeviceInfo.getLocation();
    const auto info2 = clientDeviceInfo.getLocation();
    ASSERT_EQ(serverDeviceInfo.getLocation(), clientDeviceInfo.getLocation());

    clientDevice.setName("new_name");
    clientDevice.setPropertyValue("location", "new_location");
    
    ASSERT_EQ("new_name", clientDeviceInfo.getName());
    ASSERT_EQ("new_location", clientDeviceInfo.getLocation());

    ASSERT_EQ(serverDeviceInfo.getName(), clientDeviceInfo.getName());
    ASSERT_EQ(serverDeviceInfo.getLocation(), clientDeviceInfo.getLocation());
}
