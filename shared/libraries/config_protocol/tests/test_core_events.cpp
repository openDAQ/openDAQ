#include <opendaq/component_factory.h>
#include <config_protocol/config_client_object_ptr.h>
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
#include <opendaq/device_domain_factory.h>
#include <coreobjects/property_object_factory.h>
#include "test_utils.h"
#include "config_protocol/config_protocol_server.h"
#include "config_protocol/config_protocol_client.h"
#include "config_protocol/config_client_device_impl.h"
#include <coreobjects/user_factory.h>

using namespace daq;
using namespace daq::config_protocol;

class ConfigCoreEventTest : public testing::Test
{
public:
    void SetUp() override
    {
        const auto anonymousUser = User("", "");

        serverDevice = test_utils::createTestDevice();
        server = std::make_unique<ConfigProtocolServer>(serverDevice, std::bind(&ConfigCoreEventTest::serverNotificationReady, this, std::placeholders::_1), anonymousUser);

        clientContext = NullContext();
        client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                clientContext,
                std::bind(&ConfigCoreEventTest::sendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigCoreEventTest::sendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr
            );

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
};

TEST_F(ConfigCoreEventTest, PropertyValueChanged)
{
    const auto clientComponent = client->getDevice().findComponent("IO/AI/Ch");
    const auto serverComponent = serverDevice.findComponent("IO/AI/Ch");

    int callCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyValueChanged));
            ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
            ASSERT_EQ(comp, clientComponent);
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            ASSERT_TRUE(args.getParameters().hasKey("Value"));
            ASSERT_TRUE(args.getParameters().hasKey("Path"));
            ASSERT_EQ(comp, args.getParameters().get("Owner"));

            callCount++;
        };
    
    serverComponent.setPropertyValue("StrProp", "foo");
    ASSERT_EQ(clientComponent.getPropertyValue("StrProp"), "foo");
    serverComponent.setPropertyValue("StrProp", "bar");
    ASSERT_EQ(clientComponent.getPropertyValue("StrProp"), "bar");
    serverComponent.clearPropertyValue("StrProp");
    ASSERT_EQ(clientComponent.getPropertyValue("StrProp"), "-");
    ASSERT_EQ(callCount, 3);
}

TEST_F(ConfigCoreEventTest, PropertyChangedNested)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");

    int callCount = 0;
    
    const auto obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const auto obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.child");

    clientContext.getOnCoreEvent() += [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyValueChanged));
        ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
        ASSERT_EQ(comp, clientComponent);
        ASSERT_TRUE(args.getParameters().hasKey("Value"));
        ASSERT_EQ(args.getParameters().get("Name"), "String");

        if (callCount == 0)
        {
            ASSERT_EQ(obj1, args.getParameters().get("Owner"));
            ASSERT_EQ("ObjectWithMetadata", args.getParameters().get("Path"));
        }
        else
        {
            ASSERT_EQ(obj2, args.getParameters().get("Owner"));
            ASSERT_EQ("ObjectWithMetadata.child", args.getParameters().get("Path"));
        }

        callCount++;
    };

    serverComponent.setPropertyValue("ObjectWithMetadata.String", "foo");
    serverComponent.setPropertyValue("ObjectWithMetadata.child.String", "bar");

    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.String"), "foo");
    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.child.String"), "bar");

    ASSERT_EQ(callCount, 2);
}

TEST_F(ConfigCoreEventTest, PropertyObjectUpdateEnd)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");

    int propChangeCount = 0;
    int updateCount = 0;
    int otherCount = 0;
    
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            const auto params = args.getParameters();

            if (comp != clientComponent)
                return;

            const StringPtr path = params.get("Path");
            if (path.assigned() && path != "")
                return;

            DictPtr<IString, IBaseObject> updated;
            switch (static_cast<CoreEventId>(args.getEventId()))
            {
                case CoreEventId::PropertyValueChanged:
                    propChangeCount++;
                    break;
                case CoreEventId::PropertyObjectUpdateEnd:
                    updateCount++;
                    updated = params.get("UpdatedProperties");
                    ASSERT_EQ(updated.getCount(), 2u);
                    ASSERT_EQ(args.getEventName(), "PropertyObjectUpdateEnd");
                    ASSERT_EQ(comp, params.get("Owner"));
                    break;
                default:
                    otherCount++;
                    break;
            }            
        };

    serverComponent.beginUpdate();
    serverComponent.setPropertyValue("Integer", 3);
    serverComponent.setPropertyValue("Float", 1);
    serverComponent.endUpdate();
    
    serverComponent.beginUpdate();
    serverComponent.clearPropertyValue("Integer");
    serverComponent.clearPropertyValue("Float");
    serverComponent.endUpdate();

    ASSERT_EQ(propChangeCount, 0);
    ASSERT_EQ(updateCount, 2);
    ASSERT_EQ(otherCount, 0);

    ASSERT_EQ(clientComponent.getPropertyValue("Integer"), serverComponent.getPropertyValue("Integer"));
    ASSERT_EQ(clientComponent.getPropertyValue("Float"), serverComponent.getPropertyValue("Float"));
}


TEST_F(ConfigCoreEventTest, PropertyObjectUpdateEndNested)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");

    const PropertyObjectPtr serverObj1 = serverComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr serverObj2 = serverComponent.getPropertyValue("ObjectWithMetadata.child");

    const PropertyObjectPtr obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.child");

    int updateCount = 0;
    
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            DictPtr<IString, IBaseObject> updated;
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyObjectUpdateEnd));
            updateCount++;
            updated = args.getParameters().get("UpdatedProperties");
            ASSERT_EQ(updated.getCount(), 1u);
            ASSERT_EQ(args.getEventName(), "PropertyObjectUpdateEnd");
            if (updateCount == 1)
                ASSERT_EQ(obj1, args.getParameters().get("Owner"));
            else
                ASSERT_EQ(obj2, args.getParameters().get("Owner"));
        };

    serverObj1.beginUpdate();
    serverObj1.setPropertyValue("String", "foo");
    serverObj1.endUpdate();
    
    serverObj2.beginUpdate();
    serverObj2.setPropertyValue("String", "foo");
    serverObj2.endUpdate();

    ASSERT_EQ(updateCount, 2);
    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.String"), serverComponent.getPropertyValue("ObjectWithMetadata.String"));
    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.child.String"), serverComponent.getPropertyValue("ObjectWithMetadata.child.String"));
}

TEST_F(ConfigCoreEventTest, PropertyAdded)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");

    int addCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyAdded));
            ASSERT_EQ(args.getEventName(), "PropertyAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Property"));
            ASSERT_EQ(comp, args.getParameters().get("Owner"));
            addCount++;
        };

    serverComponent.addProperty(StringProperty("test1", "foo"));
    serverComponent.addProperty(StringProperty("test2", "bar"));
    serverComponent.addProperty(FloatProperty("test3", 1.123));
    ASSERT_EQ(addCount, 3);

    ASSERT_EQ(clientComponent.getPropertyValue("test1"), serverComponent.getPropertyValue("test1"));
    ASSERT_EQ(clientComponent.getPropertyValue("test2"), serverComponent.getPropertyValue("test2"));
    ASSERT_EQ(clientComponent.getPropertyValue("test3"), serverComponent.getPropertyValue("test3"));
}

TEST_F(ConfigCoreEventTest, PropertyAddedNested)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");
    
    const PropertyObjectPtr serverObj1 = serverComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr serverObj2 = serverComponent.getPropertyValue("ObjectWithMetadata.child");

    const PropertyObjectPtr obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.child");

    int addCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyAdded));
            ASSERT_EQ(args.getEventName(), "PropertyAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Property"));

            if (addCount == 0)
                ASSERT_EQ(obj1, args.getParameters().get("Owner"));
            else
                ASSERT_EQ(obj2, args.getParameters().get("Owner"));

            addCount++;
        };
    
    serverObj1.addProperty(StringProperty("test", "foo"));
    serverObj2.addProperty(StringProperty("test", "foo"));
    ASSERT_EQ(addCount, 2);
}

TEST_F(ConfigCoreEventTest, PropertyRemoved)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");

    int removeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyRemoved));
            ASSERT_EQ(args.getEventName(), "PropertyRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            ASSERT_EQ(comp, args.getParameters().get("Owner"));
            ASSERT_EQ(comp, clientComponent);
            removeCount++;
        };

    serverComponent.removeProperty("Integer");
    serverComponent.removeProperty("Float");
    serverComponent.removeProperty("String");
    ASSERT_EQ(removeCount, 3);

    ASSERT_FALSE(clientComponent.hasProperty("Integer"));
    ASSERT_FALSE(clientComponent.hasProperty("Float"));
    ASSERT_FALSE(clientComponent.hasProperty("String"));
}

TEST_F(ConfigCoreEventTest, PropertyRemovedNested)
{
    const auto clientComponent = client->getDevice().findComponent("AdvancedPropertiesComponent");
    const auto serverComponent = serverDevice.findComponent("AdvancedPropertiesComponent");
    
    const PropertyObjectPtr serverObj1 = serverComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr serverObj2 = serverComponent.getPropertyValue("ObjectWithMetadata.child");

    const PropertyObjectPtr obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.child");

    int removeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyRemoved));
            ASSERT_EQ(args.getEventName(), "PropertyRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Name"));

            if (removeCount == 0)
                ASSERT_EQ(obj1, args.getParameters().get("Owner"));
            else
                ASSERT_EQ(obj2, args.getParameters().get("Owner"));

            removeCount++;
        };
    
    serverObj1.removeProperty("String");
    serverObj2.removeProperty("String");
    ASSERT_EQ(removeCount, 2);
}

TEST_F(ConfigCoreEventTest, ComponentAdded)
{
    int addCount = 0;
    const FolderConfigPtr clientFolder = clientDevice.getItem("FB");
    const FolderConfigPtr serverFolder = serverDevice.getItem("FB");

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentAdded));
            ASSERT_EQ(args.getEventName(), "ComponentAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Component"));
            ASSERT_EQ(args.getParameters().get("Component"), clientFolder.getItems()[addCount]);
            ASSERT_EQ(comp, clientFolder);
            addCount++;
        };

    const auto fb1 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb1");
    const auto fb2 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb2");
    const auto fb3 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb3");


    serverFolder.addItem(fb1);
    serverFolder.addItem(fb2);
    serverFolder.addItem(fb3);

    ASSERT_TRUE(clientFolder.getItem("newFb1").assigned());
    ASSERT_TRUE(clientFolder.getItem("newFb2").assigned());
    ASSERT_TRUE(clientFolder.getItem("newFb3").assigned());

    ASSERT_EQ(addCount, 3);
}

TEST_F(ConfigCoreEventTest, ServerComponentAdded)
{
    int addCount = 0;
    const FolderConfigPtr clientFolder = clientDevice.getItem("Srv");
    const FolderConfigPtr serverFolder = serverDevice.getItem("Srv");

    // test if parent assigned for existing server
    ASSERT_EQ(clientFolder.getItems()[0].asPtr<IComponent>().getParent(), clientFolder);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentAdded));
        ASSERT_EQ(args.getEventName(), "ComponentAdded");
        ASSERT_TRUE(args.getParameters().hasKey("Component"));
        ASSERT_EQ(args.getParameters().get("Component"), clientFolder.getItems()[addCount + 1]);
        ASSERT_EQ(comp, clientFolder);
        ASSERT_EQ(args.getParameters().get("Component").asPtr<IComponent>().getParent(), clientFolder);
        addCount++;
    };

    const auto srv1 =
        createWithImplementation<IServer, test_utils::MockSrvImpl>(serverDevice.getContext(), serverDevice, "newSrv1");
    const auto srv2 =
        createWithImplementation<IServer, test_utils::MockSrvImpl>(serverDevice.getContext(), serverDevice, "newSrv2");
    const auto srv3 =
        createWithImplementation<IServer, test_utils::MockSrvImpl>(serverDevice.getContext(), serverDevice, "newSrv3");

    serverFolder.addItem(srv1);
    serverFolder.addItem(srv2);
    serverFolder.addItem(srv3);

    ASSERT_TRUE(clientFolder.getItem("newSrv1").assigned());
    ASSERT_TRUE(clientFolder.getItem("newSrv2").assigned());
    ASSERT_TRUE(clientFolder.getItem("newSrv3").assigned());

    ASSERT_EQ(addCount, 3);
}

TEST_F(ConfigCoreEventTest, CustomComponentAdded)
{
    int addCount = 0;
    auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentAdded));
            ASSERT_EQ(args.getEventName(), "ComponentAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Component"));
            const auto customComps = clientDevice.getCustomComponents();
            ASSERT_EQ(args.getParameters().get("Component"), customComps[addCount + 1]);
            ASSERT_EQ(comp, clientDevice);
            addCount++;
        };

    const auto comp1 = Component(serverDevice.getContext(), serverDevice, "comp1");
    const auto comp2 = Component(serverDevice.getContext(), serverDevice, "comp2");
    const auto comp3 = Component(serverDevice.getContext(), serverDevice, "comp3");

    mock->addComponentHelper(comp1);
    mock->addComponentHelper(comp2);
    mock->addComponentHelper(comp3);

    ASSERT_TRUE(clientDevice.getItem("comp1").assigned());
    ASSERT_TRUE(clientDevice.getItem("comp2").assigned());
    ASSERT_TRUE(clientDevice.getItem("comp3").assigned());

    ASSERT_EQ(addCount, 3);
}

TEST_F(ConfigCoreEventTest, ComponentRemoved)
{
    int removeCount = 0;
    const FolderConfigPtr clientFolder = clientDevice.getItem("FB");
    const FolderConfigPtr serverFolder = serverDevice.getItem("FB");

    const auto fb1 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb1");
    const auto fb2 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb2");
    const auto fb3 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb3");

    serverFolder.addItem(fb1);
    serverFolder.addItem(fb2);
    serverFolder.addItem(fb3);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentRemoved));
            ASSERT_EQ(args.getEventName(), "ComponentRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Id"));
            ASSERT_EQ(comp, clientFolder);
            removeCount++;
        };

    serverFolder.removeItemWithLocalId("newFb1");
    serverFolder.removeItemWithLocalId("newFb2");
    serverFolder.removeItemWithLocalId("newFb3");

    ASSERT_THROW(clientFolder.getItem("newFb1"), NotFoundException);
    ASSERT_THROW(clientFolder.getItem("newFb2"), NotFoundException);
    ASSERT_THROW(clientFolder.getItem("newFb3"), NotFoundException);
    ASSERT_EQ(removeCount, 3);
}

TEST_F(ConfigCoreEventTest, ServerComponentRemoved)
{
    int removeCount = 0;
    const FolderConfigPtr clientFolder = clientDevice.getItem("Srv");
    const FolderConfigPtr serverFolder = serverDevice.getItem("Srv");

    const auto srv1 =
        createWithImplementation<IServer, test_utils::MockSrvImpl>(serverDevice.getContext(), serverDevice, "newSrv1");
    const auto srv2 =
        createWithImplementation<IServer, test_utils::MockSrvImpl>(serverDevice.getContext(), serverDevice, "newSrv2");
    const auto srv3 =
        createWithImplementation<IServer, test_utils::MockSrvImpl>(serverDevice.getContext(), serverDevice, "newSrv3");

    serverFolder.addItem(srv1);
    serverFolder.addItem(srv2);
    serverFolder.addItem(srv3);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentRemoved));
        ASSERT_EQ(args.getEventName(), "ComponentRemoved");
        ASSERT_TRUE(args.getParameters().hasKey("Id"));
        ASSERT_EQ(comp, clientFolder);
        removeCount++;
    };

    serverFolder.removeItemWithLocalId("newSrv1");
    serverFolder.removeItemWithLocalId("newSrv2");
    serverFolder.removeItemWithLocalId("newSrv3");

    ASSERT_THROW(clientFolder.getItem("newSrv1"), NotFoundException);
    ASSERT_THROW(clientFolder.getItem("newSrv2"), NotFoundException);
    ASSERT_THROW(clientFolder.getItem("newSrv3"), NotFoundException);
    ASSERT_EQ(removeCount, 3);
}

TEST_F(ConfigCoreEventTest, CustomComponentRemoved)
{
    int removeCount = 0;
    auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());
    
    const auto comp1 = Component(serverDevice.getContext(), serverDevice, "comp1");
    const auto comp2 = Component(serverDevice.getContext(), serverDevice, "comp2");
    const auto comp3 = Component(serverDevice.getContext(), serverDevice, "comp3");

    mock->addComponentHelper(comp1);
    mock->addComponentHelper(comp2);
    mock->addComponentHelper(comp3);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            removeCount++;

            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentRemoved));
            ASSERT_EQ(args.getEventName(), "ComponentRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Id"));
            ASSERT_EQ(static_cast<Int>(clientDevice.getCustomComponents().getCount()), 4 - removeCount);
            ASSERT_EQ(comp, clientDevice);
        };

    mock->removeComponentHelper("comp1");
    mock->removeComponentHelper("comp2");
    mock->removeComponentHelper("comp3");

    ASSERT_THROW(clientDevice.getItem("comp1"), NotFoundException);
    ASSERT_THROW(clientDevice.getItem("comp2"), NotFoundException);
    ASSERT_THROW(clientDevice.getItem("comp3"), NotFoundException);

    ASSERT_EQ(removeCount, 3);
}

TEST_F(ConfigCoreEventTest, SignalConnected)
{
    int connectCount = 0;
    const auto sig = serverDevice.getSignalsRecursive()[0];
    const auto ip = serverDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getInputPorts()[0];

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::SignalConnected));
            ASSERT_EQ(args.getEventName(), "SignalConnected");
            ASSERT_TRUE(args.getParameters().hasKey("Signal"));
            ASSERT_EQ(args.getParameters().get("Signal"), clientDevice.getSignalsRecursive()[0]);
            ASSERT_EQ(comp, clientDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getInputPorts()[0]);
            connectCount++;
        };


    ip.connect(sig);
    ip.connect(sig);
    ip.connect(sig);
    
    ASSERT_EQ(connectCount, 3);
}

TEST_F(ConfigCoreEventTest, SignalDisconnected)
{
    int disconnectCount = 0;
    const auto sig = serverDevice.getSignalsRecursive()[0];
    const auto ip = serverDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getInputPorts()[0];
    
    ip.connect(sig);
    ip.connect(sig);
    ip.connect(sig);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::SignalDisconnected));
            ASSERT_EQ(args.getEventName(), "SignalDisconnected");
            ASSERT_EQ(comp, clientDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getInputPorts()[0]);
            disconnectCount++;
        };

    ip.disconnect();
    ip.disconnect();
    ip.disconnect();

    
    ASSERT_EQ(disconnectCount, 1);
}

TEST_F(ConfigCoreEventTest, DataDescriptorChanged)
{
    const auto sig = serverDevice.getSignalsRecursive()[0].asPtr<ISignalConfig>();
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build();

    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::DataDescriptorChanged));
            ASSERT_EQ(args.getEventName(), "DataDescriptorChanged");
            ASSERT_TRUE(args.getParameters().hasKey("DataDescriptor"));
            ASSERT_EQ(comp, clientDevice.getSignalsRecursive()[0]);
            ASSERT_EQ(args.getParameters().get("DataDescriptor"), desc);
            changeCount++;
        };

    sig.setDescriptor(desc);
    sig.setDescriptor(desc);
    sig.setDescriptor(desc);

    ASSERT_EQ(clientDevice.getSignalsRecursive()[0].getDescriptor(), desc);
    ASSERT_EQ(changeCount, 3);
}

TEST_F(ConfigCoreEventTest, ComponentAttributeChanged)
{
    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            changeCount++;
        };

    serverDevice.asPtr<IComponentPrivate>().unlockAllAttributes();

    serverDevice.setName("name1");
    serverDevice.setDescription("desc1");
    serverDevice.setActive(false);
    serverDevice.setVisible(false);

    ASSERT_EQ(clientDevice.getName(), "name1");
    ASSERT_EQ(clientDevice.getDescription(), "desc1");
    ASSERT_EQ(clientDevice.getActive(), false);
    ASSERT_EQ(clientDevice.getVisible(), false);

    serverDevice.setName("name2");
    serverDevice.setDescription("desc2");
    serverDevice.setActive(true);
    serverDevice.setVisible(true);
    
    ASSERT_EQ(clientDevice.getName(), "name2");
    ASSERT_EQ(clientDevice.getDescription(), "desc2");
    ASSERT_EQ(clientDevice.getActive(), true);
    ASSERT_EQ(clientDevice.getVisible(), true);

    ASSERT_EQ(changeCount, 8);
}

TEST_F(ConfigCoreEventTest, ComponentActiveChangedRecursive)
{
    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            changeCount++;
        };

    serverDevice.asPtr<IComponentPrivate>().unlockAllAttributes();

    const auto components = clientDevice.getItems(search::Recursive(search::Any()));

    serverDevice.setActive(false);
    for (const auto& comp : components)
        ASSERT_FALSE(comp.getActive());

    serverDevice.setActive(true);
    for (const auto& comp : components)
        ASSERT_TRUE(comp.getActive());

    ASSERT_EQ(changeCount, 2);
}

TEST_F(ConfigCoreEventTest, ComponentActiveChangedRecursiveClientCall)
{
    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            changeCount++;
        };

    serverDevice.asPtr<IComponentPrivate>().unlockAllAttributes();

    const auto components = clientDevice.getItems(search::Recursive(search::Any()));

    clientDevice.setActive(false);
    for (const auto& comp : components)
        ASSERT_FALSE(comp.getActive());

    clientDevice.setActive(true);
    for (const auto& comp : components)
        ASSERT_TRUE(comp.getActive());

    ASSERT_EQ(changeCount, 2);
}

TEST_F(ConfigCoreEventTest, DomainSignalAttributeChanged)
{
    const FolderConfigPtr serverSigFolder = serverDevice.getItem("Sig");
    const auto domainSig = Signal(serverDevice.getContext(), serverSigFolder, "testDomainSig");

    serverSigFolder.addItem(domainSig);

    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            const FolderPtr sigFolder = clientDevice.getItem("Sig");
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            ASSERT_TRUE(args.getParameters().hasKey("DomainSignal"));
            ASSERT_EQ(args.getParameters().get("DomainSignal"), sigFolder.getItem("testDomainSig"));
            changeCount++;
        };

    const SignalConfigPtr sig = serverDevice.getSignalsRecursive()[0];
    sig.setDomainSignal(domainSig);
    sig.setDomainSignal(domainSig);

    ASSERT_EQ(changeCount, 1);
}

TEST_F(ConfigCoreEventTest, RelatedSignalsAttributeChanged)
{
    const FolderConfigPtr serverSigFolder = serverDevice.getItem("Sig");

    const auto relatedSig1 = Signal(serverDevice.getContext(), serverSigFolder, "relatedSig1");
    const auto relatedSig2 = Signal(serverDevice.getContext(), serverSigFolder, "relatedSig2");

    serverSigFolder.addItem(relatedSig1);
    serverSigFolder.addItem(relatedSig2);

    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            const FolderPtr sigFolder = clientDevice.getItem("Sig");
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            ASSERT_TRUE(args.getParameters().hasKey("RelatedSignals"));
            changeCount++;
        };

    const SignalConfigPtr sig = serverDevice.getSignalsRecursive()[0];
    const SignalPtr clientSig = clientDevice.getSignalsRecursive()[0];

    const FolderPtr clientSigFolder = clientDevice.getItem("Sig");
    const SignalPtr clientRelatedSig1 = clientSigFolder.getItem("relatedSig1");
    const SignalPtr clientRelatedSig2 = clientSigFolder.getItem("relatedSig2");

    sig.addRelatedSignal(relatedSig1);
    ASSERT_EQ(clientSig.getRelatedSignals()[0], clientRelatedSig1);

    sig.addRelatedSignal(relatedSig2);
    ASSERT_EQ(clientSig.getRelatedSignals()[1], clientRelatedSig2);
    sig.removeRelatedSignal(relatedSig2);
    ASSERT_EQ(clientSig.getRelatedSignals().getCount(), 1u);
    ASSERT_EQ(clientSig.getRelatedSignals()[0], clientRelatedSig1);

    sig.setRelatedSignals(List<ISignal>(relatedSig2));
    ASSERT_EQ(clientSig.getRelatedSignals()[0], clientRelatedSig2);

    ASSERT_EQ(changeCount, 4);
}

TEST_F(ConfigCoreEventTest, TagsChanged)
{
    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::TagsChanged));
            ASSERT_EQ(args.getEventName(), "TagsChanged");
            changeCount++;
        };

    const auto tagsPrivate = serverDevice.getTags().asPtr<ITagsPrivate>();

    tagsPrivate.add("Tag1");
    tagsPrivate.add("Tag1");
    tagsPrivate.add("Tag2");
    tagsPrivate.add("Tag2");


    ASSERT_TRUE(clientDevice.getTags().contains("Tag1"));
    ASSERT_TRUE(clientDevice.getTags().contains("Tag2"));
    ASSERT_EQ(changeCount, 2);
}

TEST_F(ConfigCoreEventTest, StatusChanged)
{
    const auto typeManager = serverDevice.getContext().getTypeManager();
    const auto statusInitValue = Enumeration("StatusType", "Status0", typeManager);
    const auto statusValue = Enumeration("StatusType", "Status1", typeManager);

    const auto statusContainer = serverDevice.getStatusContainer().asPtr<IComponentStatusContainerPrivate>();

    int changeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::StatusChanged));
        ASSERT_EQ(args.getEventName(), "StatusChanged");
        ASSERT_TRUE(args.getParameters().hasKey("TestStatus"));
        ASSERT_EQ(comp, clientDevice);

        if (changeCount % 2 == 0)
        {
            ASSERT_EQ(args.getParameters().get("TestStatus"), statusValue);
            ASSERT_EQ(args.getParameters().get("TestStatus"), "Status1");
        }
        else
        {
            ASSERT_EQ(args.getParameters().get("TestStatus"), statusInitValue);
            ASSERT_EQ(args.getParameters().get("TestStatus"), "Status0");
        }
        changeCount++;
    };
    
    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusInitValue);
    statusContainer.setStatus("TestStatus", statusValue);

    ASSERT_EQ(changeCount, 3);
}


TEST_F(ConfigCoreEventTest, TypeAdded)
{
    const auto typeManager = serverDevice.getContext().getTypeManager();

    int addCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::TypeAdded));
        ASSERT_TRUE(args.getParameters().hasKey("Type"));
        ASSERT_FALSE(comp.assigned());
        addCount++;
    };

    const auto statusType = EnumerationType("StatusType1", List<IString>("Status0", "Status1"));
    typeManager.addType(statusType);
    
    const auto structType1 = StructType("StructType1", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType1);

    const auto structType2 = StructType("StructType2", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType2);

    ASSERT_EQ(addCount, 3);

    const auto clientTypeManager = clientContext.getTypeManager();
    ASSERT_TRUE(clientTypeManager.hasType("StatusType1"));
    ASSERT_TRUE(clientTypeManager.hasType("StructType1"));
    ASSERT_TRUE(clientTypeManager.hasType("StructType2"));
}

TEST_F(ConfigCoreEventTest, TypeRemoved)
{
    const auto typeManager = serverDevice.getContext().getTypeManager();
    
    const auto clientTypeManager = clientContext.getTypeManager();

    const auto statusType = EnumerationType("StatusType1", List<IString>("Status0", "Status1"));
    typeManager.addType(statusType);
    
    const auto structType1 = StructType("StructType1", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType1);

    const auto structType2 = StructType("StructType2", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType2);

    int removeCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::TypeRemoved));
        ASSERT_TRUE(args.getParameters().hasKey("TypeName"));
        ASSERT_FALSE(comp.assigned());
        removeCount++;
    };

    typeManager.removeType("StatusType1");
    typeManager.removeType("StructType1");
    typeManager.removeType("StructType2");

    ASSERT_EQ(removeCount, 3);

    ASSERT_FALSE(clientTypeManager.hasType("StatusType1"));
    ASSERT_FALSE(clientTypeManager.hasType("StructType1"));
    ASSERT_FALSE(clientTypeManager.hasType("StructType2"));
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndValueChanged)
{
    serverDevice.addProperty(StringProperty("String", "foo"));
    serverDevice.addProperty(IntProperty("Int", 0));
    serverDevice.addProperty(FloatProperty("Float", 1.123));
    
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    serverDevice.setPropertyValue("String", "bar");
    serverDevice.setPropertyValue("Int", 1);
    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    const auto serializer = JsonSerializer();
    serverDevice.asPtr<IUpdatable>().serializeForUpdate(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    const auto str = serializer.getOutput();
    deserializer.update(serverDevice, serializer.getOutput());

    ASSERT_EQ(clientDevice.getPropertyValue("String"), serverDevice.getPropertyValue("String"));
    ASSERT_EQ(clientDevice.getPropertyValue("Int"), serverDevice.getPropertyValue("Int"));
    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndPropertyAddedRemoved)
{
    serverDevice.addProperty(FloatProperty("Temp", 1.123));

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    serverDevice.addProperty(StringProperty("String", "foo"));
    serverDevice.addProperty(IntProperty("Int", 0));
    serverDevice.addProperty(FloatProperty("Float", 1.123));
    serverDevice.removeProperty("Temp");
    
    serverDevice.setPropertyValue("String", "bar");
    serverDevice.setPropertyValue("Int", 1);
    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    
    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());

    ASSERT_EQ(clientDevice.getPropertyValue("String"), serverDevice.getPropertyValue("String"));
    ASSERT_EQ(clientDevice.getPropertyValue("Int"), serverDevice.getPropertyValue("Int"));
    ASSERT_EQ(clientDevice.getPropertyValue("Float"), serverDevice.getPropertyValue("Float"));
    ASSERT_FALSE(clientDevice.hasProperty("Temp"));

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndFbAdded)
{
    const FolderConfigPtr clientFolder = clientDevice.getItem("FB");
    const FolderConfigPtr serverFolder = serverDevice.getItem("FB");

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    const auto fb1 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb1");
    const auto fb2 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb2");
    const auto fb3 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder, "newFb3");

    serverFolder.addItem(fb1);
    serverFolder.addItem(fb2);
    serverFolder.addItem(fb3);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    
    const auto serializer = JsonSerializer();
    serverFolder.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverFolder, serializer.getOutput());

    
    ASSERT_TRUE(clientFolder.hasItem("newFb1"));
    ASSERT_TRUE(clientFolder.hasItem("newFb2"));
    ASSERT_TRUE(clientFolder.hasItem("newFb3"));

    ASSERT_TRUE(clientFolder.getItem("newFb1").supportsInterface(IFunctionBlock::Id));

    const auto clientFb1 = clientFolder.getItem("newFb1");
    const auto clientFb2 = clientFolder.getItem("newFb2");
    const auto clientFb3 = clientFolder.getItem("newFb3");
    ASSERT_EQ(fb1.getGlobalId(), clientFb1.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(fb2.getGlobalId(), clientFb2.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(fb3.getGlobalId(), clientFb3.asPtr<IConfigClientObject>().getRemoteGlobalId());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndDeviceFBModified)
{
    const FolderConfigPtr serverFolder1 = serverDevice.getItem("FB");
    const FolderConfigPtr clientFolder1 = clientDevice.getItem("FB");
    const FolderConfigPtr serverFolder2 = serverDevice.getDevices()[0].getItem("FB");
    const FolderConfigPtr clientFolder2 = clientDevice.getDevices()[0].getItem("FB");

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    serverFolder2.removeItemWithLocalId("fb");
    const auto fb1 =
        createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFolder1, "newFb1");
    serverFolder1.addItem(fb1);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    
    ASSERT_FALSE(clientFolder1.hasItem("newFb1"));
    ASSERT_TRUE(clientFolder2.hasItem("fb"));

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());

    ASSERT_TRUE(clientFolder1.hasItem("newFb1"));
    ASSERT_FALSE(clientFolder2.hasItem("fb"));

    const auto clientFb1 = clientFolder1.getItem("newFb1");
    ASSERT_EQ(fb1.getGlobalId(), clientFb1.asPtr<IConfigClientObject>().getRemoteGlobalId());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndDeviceSubDeviceChannelSignalModified)
{
    const FolderConfigPtr serverDeviceFolder = serverDevice.getItem("Dev");
    const FolderConfigPtr serverSigFolder = serverDevice.getItem("Sig");
    const FolderConfigPtr serverIOFolder = serverDevice.getItem("IO");

    const FolderConfigPtr clientDeviceFolder = clientDevice.getItem("Dev");
    const FolderConfigPtr clientSigFolder = clientDevice.getItem("Sig");
    const FolderConfigPtr clientIOFolder = clientDevice.getItem("IO");

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    
    serverDeviceFolder.removeItemWithLocalId("dev");
    serverSigFolder.removeItemWithLocalId("sig_device");
    serverIOFolder.removeItemWithLocalId("AI");
    
    const auto dev = createWithImplementation<IDevice, test_utils::MockDevice2Impl>(serverDevice.getContext(), serverDeviceFolder, "new_dev");
    const auto io = IoFolder(serverDevice.getContext(), serverIOFolder, "new_io");
    const auto sig = Signal(serverDevice.getContext(), serverSigFolder, "new_sig");

    serverDeviceFolder.addItem(dev);
    serverIOFolder.addItem(io);
    serverSigFolder.addItem(sig);

    const auto ch = createWithImplementation<IChannel, test_utils::MockChannel2Impl>(serverDevice.getContext(), io, "new_ch");
    io.addItem(ch);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    
    ASSERT_TRUE(clientDeviceFolder.hasItem("dev"));
    ASSERT_FALSE(clientDeviceFolder.hasItem("new_dev"));
    ASSERT_TRUE(clientSigFolder.hasItem("sig_device"));
    ASSERT_FALSE(clientSigFolder.hasItem("new_sig"));
    ASSERT_TRUE(clientIOFolder.hasItem("AI"));
    ASSERT_FALSE(clientIOFolder.hasItem("new_io"));

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
        
    ASSERT_FALSE(clientDeviceFolder.hasItem("dev"));
    ASSERT_TRUE(clientDeviceFolder.hasItem("new_dev"));
    ASSERT_FALSE(clientSigFolder.hasItem("sig_device"));
    ASSERT_TRUE(clientSigFolder.hasItem("new_sig"));
    ASSERT_FALSE(clientIOFolder.hasItem("AI"));
    ASSERT_TRUE(clientIOFolder.hasItem("new_io"));
    ASSERT_TRUE(clientIOFolder.getItem("new_io").asPtr<IFolder>().hasItem("new_ch"));

    const auto clientDev = clientDeviceFolder.getItem("new_dev");
    const auto clientIo = clientIOFolder.getItem("new_io");
    const auto clientSig = clientSigFolder.getItem("new_sig");
    ASSERT_EQ(dev.getGlobalId(), clientDev.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(io.getGlobalId(), clientIo.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(sig.getGlobalId(), clientSig.asPtr<IConfigClientObject>().getRemoteGlobalId());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndDeviceCustomCompModified)
{
    auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    mock->removeComponentHelper("AdvancedPropertiesComponent");

    const auto comp = Component(serverDevice.getContext(), serverDevice, "new_comp");
    const auto folder = Folder(serverDevice.getContext(), serverDevice, "new_folder");
    const auto childComp = Component(serverDevice.getContext(), folder, "new_child_comp");
    folder.addItem(childComp);

    mock->addComponentHelper(comp);
    mock->addComponentHelper(folder);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_TRUE(clientDevice.hasItem("AdvancedPropertiesComponent"));
    ASSERT_FALSE(clientDevice.hasItem("new_comp"));
    ASSERT_FALSE(clientDevice.hasItem("new_folder"));

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
    
    ASSERT_FALSE(clientDevice.hasItem("AdvancedPropertiesComponent"));
    ASSERT_TRUE(clientDevice.hasItem("new_comp"));
    ASSERT_TRUE(clientDevice.hasItem("new_folder"));
    ASSERT_TRUE(clientDevice.getItem("new_folder").asPtr<IFolder>().hasItem("new_child_comp"));

    const auto clientComp = clientDevice.getItem("new_comp");
    const FolderPtr clientFolder = clientDevice.getItem("new_folder");
    const auto clientChildComp = clientFolder.getItem("new_child_comp");
    ASSERT_EQ(comp.getGlobalId(), clientComp.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(folder.getGlobalId(), clientFolder.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(childComp.getGlobalId(), clientChildComp.asPtr<IConfigClientObject>().getRemoteGlobalId());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndFBSubFbSignalIPModified)
{
    const auto serverFB = serverDevice.getFunctionBlocks(search::Recursive(search::Any()))[0];
    const auto clientFB = clientDevice.getFunctionBlocks(search::Recursive(search::Any()))[0];

    const FolderConfigPtr serverFBFolder = serverFB.getItem("FB");
    const FolderConfigPtr serverSigFolder = serverFB.getItem("Sig");
    const FolderConfigPtr serverIPFolder = serverFB.getItem("IP");

    const FolderConfigPtr clientFBFolder = clientFB.getItem("FB");
    const FolderConfigPtr clientSigFolder = clientFB.getItem("Sig");
    const FolderConfigPtr clientIPFolder = clientFB.getItem("IP");
    
    const auto fb = createWithImplementation<IFunctionBlock, test_utils::MockFb1Impl>(serverDevice.getContext(), serverFBFolder, "fb");
    serverFBFolder.addItem(fb);

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    serverFBFolder.removeItemWithLocalId("fb");
    serverSigFolder.removeItemWithLocalId("sig1");
    serverIPFolder.removeItemWithLocalId("ip");

    const auto fbNew = createWithImplementation<IFunctionBlock, test_utils::MockFb2Impl>(serverDevice.getContext(), serverFBFolder, "new_fb");
    const auto ipNew = InputPort(serverDevice.getContext(), serverIPFolder, "new_ip");
    const auto sigNew = Signal(serverDevice.getContext(), serverSigFolder, "new_sig");

    serverFBFolder.addItem(fbNew);
    serverIPFolder.addItem(ipNew);
    serverSigFolder.addItem(sigNew);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();
    
    ASSERT_TRUE(clientFBFolder.hasItem("fb"));
    ASSERT_FALSE(clientFBFolder.hasItem("new_fb"));
    ASSERT_TRUE(clientSigFolder.hasItem("sig1"));
    ASSERT_FALSE(clientSigFolder.hasItem("new_sig"));
    ASSERT_TRUE(clientIPFolder.hasItem("ip"));
    ASSERT_FALSE(clientIPFolder.hasItem("new_ip"));

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());

    ASSERT_FALSE(clientFBFolder.hasItem("fb"));
    ASSERT_TRUE(clientFBFolder.hasItem("new_fb"));
    ASSERT_FALSE(clientSigFolder.hasItem("sig1"));
    ASSERT_TRUE(clientSigFolder.hasItem("new_sig"));
    ASSERT_FALSE(clientIPFolder.hasItem("ip"));
    ASSERT_TRUE(clientIPFolder.hasItem("new_ip"));

    const auto clientFbNew = clientFBFolder.getItem("new_fb");
    const auto clientIpNew = clientIPFolder.getItem("new_ip");
    const auto clientSigNew = clientSigFolder.getItem("new_sig");
    ASSERT_EQ(fbNew.getGlobalId(), clientFbNew.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(ipNew.getGlobalId(), clientIpNew.asPtr<IConfigClientObject>().getRemoteGlobalId());
    ASSERT_EQ(sigNew.getGlobalId(), clientSigNew.asPtr<IConfigClientObject>().getRemoteGlobalId());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndDeviceIPConnectDisconnect)
{
    const FolderConfigPtr serverFolder = serverDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getItem("IP");
    const FolderConfigPtr clientFolder = clientDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getItem("IP");
    
    const auto serverIpNew = InputPort(serverDevice.getContext(), serverFolder, "new_ip");
    const auto serverIp = serverFolder.getItem("ip").asPtr<IInputPortConfig>();

    serverFolder.addItem(serverIpNew);

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    serverIp.disconnect();
    serverIpNew.connect(serverDevice.getSignals()[0]);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_FALSE(clientFolder.getItem("new_ip").asPtr<IInputPort>().getConnection().assigned());
    ASSERT_TRUE(clientFolder.getItem("ip").asPtr<IInputPort>().getConnection().assigned());

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
    
    ASSERT_TRUE(clientFolder.getItem("new_ip").asPtr<IInputPort>().getConnection().assigned());
    ASSERT_FALSE(clientFolder.getItem("ip").asPtr<IInputPort>().getConnection().assigned());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndFBIPConnectDisconnect)
{
    const auto serverFB = serverDevice.getFunctionBlocks(search::Recursive(search::Any()))[0];
    const auto clientFB = clientDevice.getFunctionBlocks(search::Recursive(search::Any()))[0];

    const FolderConfigPtr serverFolder = serverFB.getItem("IP");
    const FolderConfigPtr clientFolder = clientFB.getItem("IP");
    
    const auto serverIpNew = InputPort(serverDevice.getContext(), serverFolder, "new_ip");
    const auto serverIp = serverFolder.getItem("ip").asPtr<IInputPortConfig>();

    serverFolder.addItem(serverIpNew);

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    serverIp.disconnect();
    serverIpNew.connect(serverDevice.getSignals()[0]);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_FALSE(clientFolder.getItem("new_ip").asPtr<IInputPort>().getConnection().assigned());
    ASSERT_TRUE(clientFolder.getItem("ip").asPtr<IInputPort>().getConnection().assigned());

    const auto serializer = JsonSerializer();
    serverFB.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverFB, serializer.getOutput());
    
    ASSERT_TRUE(clientFolder.getItem("new_ip").asPtr<IInputPort>().getConnection().assigned());
    ASSERT_FALSE(clientFolder.getItem("ip").asPtr<IInputPort>().getConnection().assigned());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndFolderIPConnectDisconnect)
{
    const auto serverDevFolder = serverDevice.getItem("Dev");
    const auto clientDevFolder = clientDevice.getItem("Dev");

    const FolderConfigPtr serverFolder = serverDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getItem("IP");
    const FolderConfigPtr clientFolder = clientDevice.getFunctionBlocks(search::Recursive(search::Any()))[0].getItem("IP");
    
    const auto serverIpNew = InputPort(serverDevice.getContext(), serverFolder, "new_ip");
    const auto serverIp = serverFolder.getItem("ip").asPtr<IInputPortConfig>();

    serverFolder.addItem(serverIpNew);

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    serverIp.disconnect();
    serverIpNew.connect(serverDevice.getSignals()[0]);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_FALSE(clientFolder.getItem("new_ip").asPtr<IInputPort>().getConnection().assigned());
    ASSERT_TRUE(clientFolder.getItem("ip").asPtr<IInputPort>().getConnection().assigned());

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
    
    ASSERT_TRUE(clientFolder.getItem("new_ip").asPtr<IInputPort>().getConnection().assigned());
    ASSERT_FALSE(clientFolder.getItem("ip").asPtr<IInputPort>().getConnection().assigned());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndDomainSignalChanged)
{
    const FolderConfigPtr serverSigFolder = serverDevice.getItem("Sig");
    const FolderConfigPtr clientSigFolder = clientDevice.getItem("Sig");

    const SignalConfigPtr serverDeviceSig = serverSigFolder.getItem("sig_device");
    const SignalConfigPtr serverDeviceTimeSig = Signal(serverDevice.getContext(), serverSigFolder, "sig_device_time");
    const SignalConfigPtr serverSubDeviceSig = serverDevice.getDevices()[0].getItem("Sig").asPtr<IFolder>().getItem("sig_device");
    
    serverSigFolder.addItem(serverDeviceTimeSig);
    serverDeviceSig.setDomainSignal(serverDeviceTimeSig);

    const SignalPtr clientDeviceSig = clientSigFolder.getItem("sig_device");
    const SignalPtr clientDeviceTimeSig = clientSigFolder.getItem("sig_device_time");
    const SignalPtr clientSubDeviceSig = clientDevice.getDevices()[0].getItem("Sig").asPtr<IFolder>().getItem("sig_device");

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    serverDeviceSig.setDomainSignal(nullptr);
    serverSubDeviceSig.setDomainSignal(serverDeviceTimeSig);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_EQ(clientDeviceSig.getDomainSignal(), clientDeviceTimeSig);
    ASSERT_EQ(clientSubDeviceSig.getDomainSignal(), nullptr);

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
    
    ASSERT_EQ(clientSubDeviceSig.getDomainSignal(), clientDeviceTimeSig);
    ASSERT_EQ(clientDeviceSig.getDomainSignal(), nullptr);

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, ComponentUpdateEndDescriptorChanged)
{
    const FolderConfigPtr serverSigFolder = serverDevice.getItem("Sig");
    const FolderConfigPtr clientSigFolder = clientDevice.getItem("Sig");

    const SignalConfigPtr serverDeviceSig = serverDevice.getItem("Sig").asPtr<IFolder>().getItem("sig_device");
    const SignalPtr clientDeviceSig = clientDevice.getItem("Sig").asPtr<IFolder>().getItem("sig_device");

    auto serverDesc = serverDeviceSig.getDescriptor();

    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    const auto newDesc = DataDescriptorBuilder().setSampleType(SampleType::ComplexFloat64).setName("Foo").build();
    serverDeviceSig.setDescriptor(newDesc);

    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_EQ(clientDeviceSig.getDescriptor(), serverDesc);

    const auto serializer = JsonSerializer();
    serverDevice.serialize(serializer);
    const auto out = serializer.getOutput();

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(serverDevice, serializer.getOutput());
    
    ASSERT_EQ(clientDeviceSig.getDescriptor(), newDesc);

    ASSERT_EQ(updateCount, 1);
}

TEST_F(ConfigCoreEventTest, DomainChanged)
{
    int changeCount = 0;
    auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::DeviceDomainChanged));
            ASSERT_EQ(args.getEventName(), "DeviceDomainChanged");
            ASSERT_TRUE(args.getParameters().hasKey("DeviceDomain"));
            ASSERT_EQ(comp, clientDevice);
            changeCount++;
        };

    DeviceDomainPtr domain1 = DeviceDomain(Ratio(1, 1), "foo", Unit("test"));
    DeviceDomainPtr domain2 = DeviceDomain(Ratio(1, 1), "bar", Unit("test1"));

    mock->setDeviceDomainHelper(domain1);
    ASSERT_EQ(clientDevice.getDomain().getOrigin(), "foo");

    mock->setDeviceDomainHelper(domain2);
    ASSERT_EQ(clientDevice.getDomain().getOrigin(), "bar");

    mock->setDeviceDomainHelper(domain1);
    ASSERT_EQ(clientDevice.getDomain().getOrigin(), "foo");

    ASSERT_EQ(changeCount, 3);
}

TEST_F(ConfigCoreEventTest, ReconnectComponentUpdateEnd)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    serverDevice.addProperty(StringProperty("String", "foo"));

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
        updateCount++;
    };

    client->reconnect(False);
    ASSERT_EQ(updateCount, 1);

    ASSERT_EQ(clientDevice.getPropertyValue("String"), serverDevice.getPropertyValue("String"));
}

TEST_F(ConfigCoreEventTest, ReconnectRestoreClientConfig)
{
    serverDevice.addProperty(StringProperty("String", "foo"));
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    serverDevice.setPropertyValue("String", "test");
    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    ASSERT_EQ(serverDevice.getPropertyValue("String"), "test");
    ASSERT_EQ(clientDevice.getPropertyValue("String"), "foo");

    int updateCount = 0;
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
        updateCount++;
    };

    client->reconnect(True);
    ASSERT_EQ(updateCount, 1);

    ASSERT_EQ(clientDevice.getPropertyValue("String"), "foo");
    ASSERT_EQ(serverDevice.getPropertyValue("String"), "foo");
}

TEST_F(ConfigCoreEventTest, ReconnectComponentUpdateEndDeviceInfo)
{
    serverDevice.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    serverDevice.addProperty(StringProperty("String", "foo"));

    const auto info = DeviceInfo(serverDevice.getInfo().getConnectionString(), "");
    info.setManufacturer("test");
    info.setSerialNumber("test");
    auto mock = dynamic_cast<test_utils::MockDevice2Impl*>(serverDevice.getObject());
    mock->setDeviceInfoHelper(info);

    const auto infoTest = serverDevice.getInfo();

    client->reconnect(False);

    ASSERT_EQ(clientDevice.getInfo().getSerialNumber(), "test");
    ASSERT_EQ(clientDevice.getInfo().getManufacturer(), "test");
}
