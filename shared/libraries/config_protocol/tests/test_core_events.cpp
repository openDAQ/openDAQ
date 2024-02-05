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
#include "test_utils.h"
#include "config_protocol/config_protocol_server.h"
#include "config_protocol/config_protocol_client.h"

using namespace daq;
using namespace daq::config_protocol;

class ConfigCoreEventTest : public testing::Test
{
public:
    void SetUp() override
    {
        serverDevice = test_utils::createServerDevice();
        serverDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(serverDevice, std::bind(&ConfigCoreEventTest::serverNotificationReady, this, std::placeholders::_1));

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient>(clientContext, std::bind(&ConfigCoreEventTest::sendRequest, this, std::placeholders::_1), nullptr);

        client->connect();
        client->getDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        clientDevice = client->getDevice();
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient> client;
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

TEST_F(ConfigCoreEventTest, PropertyValueChanged)
{
    const auto clientComponent = client->getDevice().findComponent("IO/ai/ch");
    const auto serverComponent = serverDevice.findComponent("IO/ai/ch");

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
    const auto obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.Child");

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
            ASSERT_EQ("ObjectWithMetadata.Child", args.getParameters().get("Path"));
        }

        callCount++;
    };

    serverComponent.setPropertyValue("ObjectWithMetadata.String", "foo");
    serverComponent.setPropertyValue("ObjectWithMetadata.Child.String", "bar");

    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.String"), "foo");
    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.Child.String"), "bar");

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
            DictPtr<IString, IBaseObject> updated;
            switch (static_cast<CoreEventId>(args.getEventId()))
            {
                case CoreEventId::PropertyValueChanged:
                    propChangeCount++;
                    break;
                case CoreEventId::PropertyObjectUpdateEnd:
                    updateCount++;
                    updated = args.getParameters().get("UpdatedProperties");
                    ASSERT_EQ(updated.getCount(), 2);
                    ASSERT_EQ(args.getEventName(), "PropertyObjectUpdateEnd");
                    ASSERT_EQ(comp, args.getParameters().get("Owner"));
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
    const PropertyObjectPtr serverObj2 = serverComponent.getPropertyValue("ObjectWithMetadata.Child");

    const PropertyObjectPtr obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.Child");

    int updateCount = 0;
    
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            DictPtr<IString, IBaseObject> updated;
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyObjectUpdateEnd));
            updateCount++;
            updated = args.getParameters().get("UpdatedProperties");
            ASSERT_EQ(updated.getCount(), 1);
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
    ASSERT_EQ(clientComponent.getPropertyValue("ObjectWithMetadata.Child.String"), serverComponent.getPropertyValue("ObjectWithMetadata.Child.String"));
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
    const PropertyObjectPtr serverObj2 = serverComponent.getPropertyValue("ObjectWithMetadata.Child");

    const PropertyObjectPtr obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.Child");

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
    const PropertyObjectPtr serverObj2 = serverComponent.getPropertyValue("ObjectWithMetadata.Child");

    const PropertyObjectPtr obj1 = clientComponent.getPropertyValue("ObjectWithMetadata");
    const PropertyObjectPtr obj2 = clientComponent.getPropertyValue("ObjectWithMetadata.Child");

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
            ASSERT_EQ(clientDevice.getCustomComponents().getCount(), 4 - removeCount);
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

TEST_F(ConfigCoreEventTest, ComponentUpdateEnd)
{
    
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
    ASSERT_EQ(clientSig.getRelatedSignals().getCount(), 1);
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
