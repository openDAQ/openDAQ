#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/instance_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/folder_config_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/component_status_container_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/sync_component_ptr.h>
#include <opendaq/sync_component_private_ptr.h>
#include "test_utils.h"
#include "config_protocol/config_protocol_server.h"
#include "config_protocol/config_protocol_client.h"
#include "config_protocol/config_client_device_impl.h"

using namespace daq;
using namespace daq::config_protocol;

class ConfigNestedPropertyObjectTest : public testing::Test
{
public:
    void SetUp() override
    {
        serverDevice = test_utils::createServerDevice();
        serverDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(serverDevice, std::bind(&ConfigNestedPropertyObjectTest::serverNotificationReady, this, std::placeholders::_1), nullptr);

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(clientContext, std::bind(&ConfigNestedPropertyObjectTest::sendRequest, this, std::placeholders::_1), nullptr);

        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    ContextPtr clientContext;
    BaseObjectPtr notificationObj;

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        client->triggerNotificationPacket(notificationPacket);
    }

    // client handling
    PacketBuffer sendRequest(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
    }
};

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientGet)
{
    const PropertyObjectPtr objectProperty = clientDevice.getPropertyValue("ObjectProperty");
    const PropertyObjectPtr child1 = objectProperty.getPropertyValue("child1");
    const PropertyObjectPtr child2 = objectProperty.getPropertyValue("child2");
    const PropertyObjectPtr child1_1 = child1.getPropertyValue("child1_1");
    const PropertyObjectPtr child1_2 = child1.getPropertyValue("child1_2");
    const PropertyObjectPtr child1_2_1 = child1_2.getPropertyValue("child1_2_1");
    const PropertyObjectPtr child2_1 = child2.getPropertyValue("child2_1");

    ASSERT_EQ(child1_2_1.getPropertyValue("String"), "String");
    ASSERT_DOUBLE_EQ(child1_1.getPropertyValue("Float"), 1.1);
    ASSERT_EQ(child1_2.getPropertyValue("Int"), 1);
    ASSERT_EQ(child2_1.getPropertyValue("Ratio"), Ratio(1,2));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientGetDotAccess)
{
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 2));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientSet)
{
    const PropertyObjectPtr objectProperty = clientDevice.getPropertyValue("ObjectProperty");
    const PropertyObjectPtr child1 = objectProperty.getPropertyValue("child1");
    const PropertyObjectPtr child2 = objectProperty.getPropertyValue("child2");
    const PropertyObjectPtr child1_1 = child1.getPropertyValue("child1_1");
    const PropertyObjectPtr child1_2 = child1.getPropertyValue("child1_2");
    const PropertyObjectPtr child1_2_1 = child1_2.getPropertyValue("child1_2_1");
    const PropertyObjectPtr child2_1 = child2.getPropertyValue("child2_1");
    
    child1_2_1.setPropertyValue("String", "new_string");
    child1_1.setPropertyValue("Float", 2.1);
    child1_2.setPropertyValue("Int", 2);
    child2_1.setPropertyValue("Ratio", Ratio(1, 5));

    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));

    ASSERT_EQ(child1_2_1.getPropertyValue("String"), "new_string");
    ASSERT_DOUBLE_EQ(child1_1.getPropertyValue("Float"), 2.1);
    ASSERT_EQ(child1_2.getPropertyValue("Int"), 2);
    ASSERT_EQ(child2_1.getPropertyValue("Ratio"), Ratio(1, 5));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientSetDotAccess)
{
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    clientDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
    
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientProtectedSet)
{
    ASSERT_THROW(clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.ReadOnlyString", "new_string"), AccessDeniedException);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.ReadOnlyString"), "String");
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.ReadOnlyString"), "String");

    ASSERT_NO_THROW(clientDevice.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.ReadOnlyString", "new_string"));
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.ReadOnlyString"), "new_string");
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.ReadOnlyString"), "new_string");
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientClear)
{
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    clientDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));

    clientDevice.clearPropertyValue("ObjectProperty");

    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 2));

    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 2));

    clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    clientDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    clientDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    
    clientDevice.clearPropertyValue("ObjectProperty.child1");

    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(serverDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));

    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientClassProperty)
{
    clientDevice.setPropertyValue("MockChild.NestedStringProperty", "new_string");
    ASSERT_EQ(serverDevice.getPropertyValue("MockChild.NestedStringProperty"), "new_string");
    ASSERT_EQ(clientDevice.getPropertyValue("MockChild.NestedStringProperty"), "new_string");
    clientDevice.clearPropertyValue("MockChild.NestedStringProperty");
    ASSERT_EQ(serverDevice.getPropertyValue("MockChild.NestedStringProperty"), "String");
    ASSERT_EQ(clientDevice.getPropertyValue("MockChild.NestedStringProperty"), "String");
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectServerSet)
{
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    serverDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectServerClearIndividual)
{
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    serverDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));

    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
    
    serverDevice.clearPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String");
    serverDevice.clearPropertyValue("ObjectProperty.child1.child1_1.Float");
    serverDevice.clearPropertyValue("ObjectProperty.child1.child1_2.Int");
    serverDevice.clearPropertyValue("ObjectProperty.child2.child2_1.Ratio");

    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 2));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectServerClearObject)
{
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    serverDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));

    serverDevice.clearPropertyValue("ObjectProperty");

    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 2));
    
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    serverDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));

    serverDevice.clearPropertyValue("ObjectProperty.child1");
    
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectServerUpdate)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
    serverDevice.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
    serverDevice.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
    
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.Int"), 2);
    ASSERT_EQ(clientDevice.getPropertyValue("ObjectProperty.child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientFunctionCall)
{
    const PropertyObjectPtr child = clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1");
    FunctionPtr func1 = clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.Function");
    FunctionPtr func2 = child.getPropertyValue("Function");

    ASSERT_EQ(func1(1), 1);
    ASSERT_EQ(func2(5), 5);
}

TEST_F(ConfigNestedPropertyObjectTest, TestNestedObjectClientProcedureCall)
{
    const PropertyObjectPtr child = clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1");
    ProcedurePtr proc1 = clientDevice.getPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.Procedure");
    ProcedurePtr proc2 = child.getPropertyValue("Procedure");

    ASSERT_NO_THROW(proc1(5));
    ASSERT_THROW(proc1(0), InvalidParameterException);
    ASSERT_NO_THROW(proc2(5));
    ASSERT_THROW(proc2(0), InvalidParameterException);
}

TEST_F(ConfigNestedPropertyObjectTest, TestSyncComponent)
{
    auto typeManager = serverDevice.getContext().getTypeManager();

    // update the sync component in the server side
    SyncComponentPtr syncComponent = serverDevice.getSyncComponent();
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);
    ASSERT_ANY_THROW(syncComponentPrivate.addInterface(PropertyObject(typeManager, "SyncInterfaceBase")));
    syncComponentPrivate.addInterface(PropertyObject(typeManager, "PtpSyncInterface"));
    syncComponentPrivate.addInterface(PropertyObject(typeManager, "InterfaceClockSync"));
    syncComponent.setSelectedSource(1);
    syncComponentPrivate.setSyncLocked(true);

    // check that the client side has the same sync component
    SyncComponentPtr clientSyncComponent = clientDevice.getSyncComponent();
    ASSERT_EQ(clientSyncComponent.getSelectedSource(), 1);
    ASSERT_EQ(clientSyncComponent.getInterfaces().getCount(), 2);
    ASSERT_EQ(clientSyncComponent.getInterfaceNames().getCount(), 2);
    ASSERT_EQ(clientSyncComponent.getInterfaceNames(), syncComponent.getInterfaceNames());
    ASSERT_EQ(clientSyncComponent.getSyncLocked(), true);

    // update the sync component in the client side
    clientSyncComponent.setSelectedSource(0);

    ASSERT_EQ(clientSyncComponent.getSelectedSource(), 0); 

    // check that the server side has the same sync component
    ASSERT_EQ(syncComponent.getSelectedSource(), 0);    
}


TEST_F(ConfigNestedPropertyObjectTest, SyncComponentCustomInterfaceValues)
{
    auto typeManager = serverDevice.getContext().getTypeManager();

    // update the sync component in the server side
    SyncComponentPtr syncComponent = serverDevice.getSyncComponent();
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    auto ptpSyncInterface = PropertyObject(typeManager, "PtpSyncInterface");
    ptpSyncInterface.setPropertyValue("Mode", 2);
    
    PropertyObjectPtr status = ptpSyncInterface.getPropertyValue("Status");
    status.setPropertyValue("State", 2);
    status.setPropertyValue("Grandmaster", "1234");

    PropertyObjectPtr parameters = ptpSyncInterface.getPropertyValue("Parameters");

    StructPtr configuration = parameters.getPropertyValue("PtpConfigurationStructure");

    auto newConfiguration = StructBuilder(configuration)
                    .set("ClockType",  Enumeration("PtpClockTypeEnumeration", "OrdinaryBoundary", typeManager))
                    .set("TransportProtocol", Enumeration("PtpProtocolEnumeration", "UDP6_SCOPE", typeManager))
                    .set("StepFlag", Enumeration("PtpStepFlagEnumeration", "Two", typeManager))
                    .set("DomainNumber", 123)
                    .set("LeapSeconds", 123)
                    .set("DelayMechanism", Enumeration("PtpDelayMechanismEnumeration", "E2E", typeManager))
                    .set("Priority1", 123)
                    .set("Priority2", 123)
                    .set("Profiles", Enumeration("PtpProfileEnumeration", "802_1AS", typeManager))
                    .build();

    parameters.setPropertyValue("PtpConfigurationStructure", newConfiguration);

    PropertyObjectPtr ports = parameters.getPropertyValue("Ports");
    ports.addProperty(BoolProperty("Port1", true));
        
    syncComponentPrivate.addInterface(ptpSyncInterface);

    SyncComponentPtr clientSyncComponent = clientDevice.getSyncComponent();

    auto serverInterfaces = syncComponent.getInterfaces();
    auto clientInterfaces = clientSyncComponent.getInterfaces();
    ASSERT_EQ(serverInterfaces.getCount(), clientInterfaces.getCount());

    auto clientPtpSyncInterface = clientInterfaces.get("PtpSyncInterface");
    ASSERT_EQ(clientPtpSyncInterface.getPropertyValue("Mode"), 2);

    PropertyObjectPtr clientStatus = clientPtpSyncInterface.getPropertyValue("Status");
    ASSERT_EQ(clientStatus.getPropertyValue("State"), 2);
    ASSERT_EQ(clientStatus.getPropertyValue("Grandmaster"), "1234");

    PropertyObjectPtr clientParameters = clientPtpSyncInterface.getPropertyValue("Parameters");
    ASSERT_EQ(clientParameters.getPropertyValue("PtpConfigurationStructure"), newConfiguration);
    
    PropertyObjectPtr clientPorts = clientParameters.getPropertyValue("Ports");
    ASSERT_EQ(clientPorts.getPropertyValue("Port1"), true);
}