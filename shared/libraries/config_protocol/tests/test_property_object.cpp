#include <opendaq/component_factory.h>
#include <config_protocol/config_client_object_ptr.h>
#include <gtest/gtest.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>
#include <opendaq/instance_factory.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;
using namespace std::placeholders;

class ConfigProtocolPropertyObjectTest : public Test
{
public:
    void SetUp() override
    {
        auto anonymousUser = User("", "");
        auto instance = Instance("[[none]]");
        setUpRootProperties(instance.getRootDevice());

        serverDevice = instance.getRootDevice();
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(
            serverDevice,
            std::bind(&ConfigProtocolPropertyObjectTest::serverNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(serverDevice.getContext()));

        clientContext = NullContext();
        client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                clientContext,
                std::bind(&ConfigProtocolPropertyObjectTest::sendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigProtocolPropertyObjectTest::sendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }
    
    static void setUpRootProperties(const PropertyObjectPtr& obj)
    {
        obj.addProperty(SparseSelectionProperty("SparseSelectionInt", Dict<IInteger, IInteger>({{0, 10}, {5, 20}}), 5));
        obj.addProperty(SparseSelectionProperty("SparseSelectionString", Dict<IInteger, IString>({{0, "foo"}, {10, "bar"}}), 0));

        obj.addProperty(SelectionProperty("IndexSelectionInt", List<IInteger>(10, 20, 30), 1));
        obj.addProperty(SelectionProperty("IndexSelectionString", List<IString>("foo", "bar"), 0));

		obj.addProperty(StringPropertyBuilder("StringSelection", "foo").setSelectionValues(List<IString>("foo", "bar")).build());
		obj.addProperty(IntPropertyBuilder("IntSelection", 10).setSelectionValues(List<IInteger>(0, 6, 15, 10)).setIsValueSelectionProperty(true).build());
		obj.addProperty(FloatPropertyBuilder("FloatSelection", 5.12).setSelectionValues(List<IFloat>(0.12, -5.2, 5.12, 10.2)).build());
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

TEST_F(ConfigProtocolPropertyObjectTest, SelectionTypes)
{
    ASSERT_EQ(clientDevice.getProperty("SparseSelectionInt").getPropertyType(), PropertyType::SparseSelection);
    ASSERT_EQ(clientDevice.getProperty("SparseSelectionString").getPropertyType(), PropertyType::SparseSelection);
    ASSERT_EQ(clientDevice.getProperty("IndexSelectionInt").getPropertyType(), PropertyType::IndexSelection);
    ASSERT_EQ(clientDevice.getProperty("IndexSelectionString").getPropertyType(), PropertyType::IndexSelection);
    ASSERT_EQ(clientDevice.getProperty("StringSelection").getPropertyType(), PropertyType::Selection);
    ASSERT_EQ(clientDevice.getProperty("IntSelection").getPropertyType(), PropertyType::Selection);
    ASSERT_EQ(clientDevice.getProperty("FloatSelection").getPropertyType(), PropertyType::Selection);
}

TEST_F(ConfigProtocolPropertyObjectTest, SelectionValueTypes)
{
    ASSERT_EQ(clientDevice.getProperty("SparseSelectionInt").getValueType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("SparseSelectionString").getValueType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("IndexSelectionInt").getValueType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("IndexSelectionString").getValueType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("StringSelection").getValueType(), CoreType::ctString);
	ASSERT_EQ(clientDevice.getProperty("IntSelection").getValueType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("FloatSelection").getValueType(), CoreType::ctFloat);

}

TEST_F(ConfigProtocolPropertyObjectTest, SelectionKeyTypes)
{
    ASSERT_EQ(clientDevice.getProperty("SparseSelectionInt").getKeyType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("SparseSelectionString").getKeyType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("IndexSelectionInt").getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(clientDevice.getProperty("IndexSelectionString").getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(clientDevice.getProperty("StringSelection").getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(clientDevice.getProperty("IntSelection").getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(clientDevice.getProperty("FloatSelection").getKeyType(), CoreType::ctUndefined);
}

TEST_F(ConfigProtocolPropertyObjectTest, SelectionItemTypes)
{
    ASSERT_EQ(clientDevice.getProperty("SparseSelectionInt").getItemType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("SparseSelectionString").getItemType(), CoreType::ctString);
	ASSERT_EQ(clientDevice.getProperty("IndexSelectionInt").getItemType(), CoreType::ctInt);
	ASSERT_EQ(clientDevice.getProperty("IndexSelectionString").getItemType(), CoreType::ctString);
	ASSERT_EQ(clientDevice.getProperty("StringSelection").getItemType(), CoreType::ctUndefined);
	ASSERT_EQ(clientDevice.getProperty("IntSelection").getItemType(), CoreType::ctUndefined);
	ASSERT_EQ(clientDevice.getProperty("FloatSelection").getItemType(), CoreType::ctUndefined);
}

TEST_F(ConfigProtocolPropertyObjectTest, ValidWriteValueBasedSelection)
{
    auto stringSelection = clientDevice.getProperty("StringSelection");
    ASSERT_NO_THROW(stringSelection.setValue("bar"));
    ASSERT_EQ(stringSelection.getValue(), "bar");
    ASSERT_NO_THROW(clientDevice.setPropertyValue("StringSelection", "foo"));
    ASSERT_EQ(clientDevice.getPropertyValue("StringSelection"), "foo");
    
    auto intSelection = clientDevice.getProperty("IntSelection");
    ASSERT_NO_THROW(intSelection.setValue(6));
    ASSERT_EQ(intSelection.getValue(), 6);
    ASSERT_NO_THROW(clientDevice.setPropertyValue("IntSelection", 10));
    ASSERT_EQ(clientDevice.getPropertyValue("IntSelection"), 10);
    
    auto floatSelection = clientDevice.getProperty("FloatSelection");
    ASSERT_NO_THROW(floatSelection.setValue(10.2));
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 10.2);
    ASSERT_NO_THROW(clientDevice.setPropertyValue("FloatSelection", -5.2));
    ASSERT_DOUBLE_EQ(clientDevice.getPropertyValue("FloatSelection"), -5.2);
}

TEST_F(ConfigProtocolPropertyObjectTest, InvalidWriteValueBasedSelection)
{
    auto stringSelection = clientDevice.getProperty("StringSelection");
    ASSERT_THROW(stringSelection.setValue("foobar"), NotFoundException);
    ASSERT_THROW(clientDevice.setPropertyValue("StringSelection", "foobar"), NotFoundException);

    auto intSelection = clientDevice.getProperty("IntSelection");
    ASSERT_THROW(intSelection.setValue(5), NotFoundException);
    ASSERT_THROW(clientDevice.setPropertyValue("IntSelection", 5), NotFoundException);

    auto floatSelection = clientDevice.getProperty("FloatSelection");
    ASSERT_THROW(floatSelection.setValue(0.5), NotFoundException);
    ASSERT_THROW(clientDevice.setPropertyValue("FloatSelection", 0.5), NotFoundException);
}

TEST_F(ConfigProtocolPropertyObjectTest, BeginEndUpdateValueBasedSelection)
{
    clientDevice.beginUpdate();

    auto stringSelection = clientDevice.getProperty("StringSelection");
    ASSERT_NO_THROW(stringSelection.setValue("bar"));
    
    auto intSelection = clientDevice.getProperty("IntSelection");
    ASSERT_NO_THROW(intSelection.setValue(6));
    
    auto floatSelection = clientDevice.getProperty("FloatSelection");
    ASSERT_NO_THROW(floatSelection.setValue(10.2));

    clientDevice.endUpdate();
    
    ASSERT_EQ(stringSelection.getValue(), "bar");
    ASSERT_EQ(intSelection.getValue(), 6);
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 10.2);
}

TEST_F(ConfigProtocolPropertyObjectTest, UpdatableUpdateValueBasedSelection)
{   
    auto stringSelection = clientDevice.getProperty("StringSelection");
    ASSERT_NO_THROW(stringSelection.setValue("bar"));
    
    auto intSelection = clientDevice.getProperty("IntSelection");
    ASSERT_NO_THROW(intSelection.setValue(6));
    
    auto floatSelection = clientDevice.getProperty("FloatSelection");
    ASSERT_NO_THROW(floatSelection.setValue(10.2));

    auto ser = JsonSerializer();
    clientDevice.serialize(ser);

    clientDevice.clearPropertyValue("StringSelection");
    clientDevice.clearPropertyValue("IntSelection");
    clientDevice.clearPropertyValue("FloatSelection");

    auto deser = JsonDeserializer();
    deser.update(clientDevice, ser.getOutput());
    
    ASSERT_EQ(stringSelection.getValue(), "bar");
    ASSERT_EQ(intSelection.getValue(), 6);
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 10.2);
}
