#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>

using namespace daq;
using namespace daq::config_protocol;

class ConfigRemoteUpdateTest : public testing::Test
{
public:
    void SetUp() override
    {
        const auto anonymousUser = User("", "");

        referenceDevice = test_utils::createTestDevice("root_dev", true);
        setUpDevice(referenceDevice);
        serverDevice = test_utils::createTestDevice("root_dev", true);
        server =
            std::make_unique<ConfigProtocolServer>(serverDevice,
                                                   std::bind(&ConfigRemoteUpdateTest::serverNotificationReady, this, std::placeholders::_1),
                                                   anonymousUser,
                                                   ClientType::Control,
                                                   test_utils::dummyExtSigFolder(serverDevice.getContext()));

        clientContext = NullContext();
        client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                clientContext,
                std::bind(&ConfigRemoteUpdateTest::sendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigRemoteUpdateTest::sendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    DevicePtr referenceDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    ContextPtr clientContext;
    BaseObjectPtr notificationObj;
    bool muteNotifications = false;
    // JSON of every RPC request the client sent through sendRequestAndGetReply
    mutable std::vector<std::string> rpcRequests;
    // Requests the client sent while it was handling a server notification. The native client
    // handles notifications and delivers replies on one thread, so such a request can never be
    // answered there; the fixture answers it with an error instead of re-entering the server.
    mutable std::vector<std::string> requestsDuringNotification;
    mutable bool deliveringNotification = false;
    // Optional hook that replaces a server notification packet before the client sees it
    std::function<PacketBuffer(const PacketBuffer&)> notificationRewriter;

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        if (muteNotifications)
            return;

        deliveringNotification = true;
        try
        {
            if (notificationRewriter)
                client->triggerNotificationPacket(notificationRewriter(notificationPacket));
            else
                client->triggerNotificationPacket(notificationPacket);
        }
        catch (...)
        {
            deliveringNotification = false;
            throw;
        }
        deliveringNotification = false;
    }

    // client handling
    PacketBuffer sendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        if (requestPacket.getPacketType() == config_protocol::PacketType::Rpc)
        {
            const auto json = requestPacket.parseRpcRequestOrReply().toStdString();
            rpcRequests.push_back(json);

            if (deliveringNotification)
            {
                requestsDuringNotification.push_back(json);
                auto error = Dict<IString, IBaseObject>();
                error.set("ErrorCode", static_cast<Int>(OPENDAQ_ERR_GENERALERROR));
                error.set("ErrorMessage", "Request sent while the client was handling a notification");
                const auto serializer = JsonSerializer();
                error.serialize(serializer);
                const auto reply = serializer.getOutput();
                return PacketBuffer::createRpcRequestOrReply(requestPacket.getId(), reply.getCharPtr(), reply.getLength());
            }
        }
        return server->processRequestAndGetReply(requestPacket);
    }

    void sendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        server->processNoReplyRequest(requestPacket);
    }

    void setUpDevice(DevicePtr& device)
    {
        device.setPropertyValue("MockString", "new_string");
        device.getCustomComponents()[0].setPropertyValue("Ratio", Ratio(1, 2000));
        const auto dev = device.getDevices()[0];
        dev.getChannels()[0].setPropertyValue("TestStringProp", "test1");
        dev.getFunctionBlocks()[0].setPropertyValue("MockString", "new_string");
        dev.getFunctionBlocks()[0].getInputPorts()[0].connect(dev.getSignals()[0]);

        device.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
        device.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
        device.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
        device.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    }

    StringPtr serializeHelper(const SerializablePtr& serializable)
    {
        const auto serializer = JsonSerializer();
        serializable.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }

    void updateHelper(const UpdatablePtr& updatable, const StringPtr& str)
    {
        const auto deserializer = JsonDeserializer();
        deserializer.update(updatable, str);
    }
};

TEST_F(ConfigRemoteUpdateTest, TestDeviceUpdate1)
{
    const auto str = serializeHelper(referenceDevice);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(serializeHelper(comp), str);
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentUpdateEnd));
        };

    updateHelper(clientDevice, str);
    ASSERT_EQ(serializeHelper(serverDevice), str);
    ASSERT_EQ(serializeHelper(clientDevice), str);
}

TEST_F(ConfigRemoteUpdateTest, TestDeviceUpdate2)
{
    const auto str = serializeHelper(referenceDevice.getDevices()[0]);
    updateHelper(clientDevice.getDevices()[0], str);
    ASSERT_EQ(serializeHelper(serverDevice.getDevices()[0]), str);
    ASSERT_EQ(serializeHelper(clientDevice.getDevices()[0]), str);
}

TEST_F(ConfigRemoteUpdateTest, TestFbUpdate)
{
    const auto str = serializeHelper(referenceDevice.getDevices()[0].getFunctionBlocks()[0]);
    updateHelper(clientDevice.getDevices()[0].getFunctionBlocks()[0], str);
    ASSERT_EQ(serializeHelper(serverDevice.getDevices()[0].getFunctionBlocks()[0]), str);
    ASSERT_EQ(serializeHelper(clientDevice.getDevices()[0].getFunctionBlocks()[0]), str);
}

TEST_F(ConfigRemoteUpdateTest, TestNestedPropertyObjectUpdate1)
{
    const auto str = serializeHelper(referenceDevice.getPropertyValue("ObjectProperty"));
    updateHelper(clientDevice.getPropertyValue("ObjectProperty"), str);
    ASSERT_EQ(serializeHelper(serverDevice.getPropertyValue("ObjectProperty")), str);
    ASSERT_EQ(serializeHelper(clientDevice.getPropertyValue("ObjectProperty")), str);
}

TEST_F(ConfigRemoteUpdateTest, TestNestedPropertyObjectUpdate2)
{
    const auto str = serializeHelper(referenceDevice.getPropertyValue("ObjectProperty.child1"));
    updateHelper(clientDevice.getPropertyValue("ObjectProperty.child1"), str);
    ASSERT_EQ(serializeHelper(serverDevice.getPropertyValue("ObjectProperty.child1")), str);
    ASSERT_EQ(serializeHelper(clientDevice.getPropertyValue("ObjectProperty.child1")), str);
}

TEST_F(ConfigRemoteUpdateTest, TestNestedPropertyObjectUpdate3)
{
    const auto str = serializeHelper(referenceDevice.getPropertyValue("ObjectProperty.child1.child1_2"));
    updateHelper(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2"), str);
    ASSERT_EQ(serializeHelper(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2")), str);
    ASSERT_EQ(serializeHelper(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2")), str);
}

TEST_F(ConfigRemoteUpdateTest, TestClientSideSerializedString)
{
    int callCount = 0;   
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentUpdateEnd));
            callCount++;
        };

    const auto strReference = serializeHelper(referenceDevice);
    const auto strDefault = serializeHelper(serverDevice);

    updateHelper(clientDevice, strReference);
    const auto strClient = serializeHelper(clientDevice);

    updateHelper(serverDevice, strDefault);
    ASSERT_EQ(serializeHelper(serverDevice), strDefault);
    ASSERT_EQ(serializeHelper(clientDevice), strDefault);

    updateHelper(clientDevice, strClient);
    ASSERT_EQ(serializeHelper(serverDevice), strClient);
    ASSERT_EQ(serializeHelper(clientDevice), strClient);

    ASSERT_EQ(callCount, 3);
}

TEST_F(ConfigRemoteUpdateTest, TestRemoveStaticComponents)
{
    auto dev = clientDevice.getDevices()[1];
    ASSERT_THROW(clientDevice.removeDevice(dev), InvalidOperationException);

    auto fb = clientDevice.getFunctionBlocks()[0];
    ASSERT_THROW(clientDevice.removeFunctionBlock(fb), InvalidOperationException);
}

TEST_F(ConfigRemoteUpdateTest, UpdateReplacesChangedProperty)
{
    auto serverComponent = serverDevice.getCustomComponents()[0];
    auto clientComponent = clientDevice.getCustomComponents()[0];

    // Replace a property on the server while notifications are muted; the client keeps the old metadata
    muteNotifications = true;
    serverComponent.removeProperty("Ratio");
    serverComponent.addProperty(RatioPropertyBuilder("Ratio", Ratio(1, 10)).setDescription("changed").build());
    muteNotifications = false;

    ASSERT_EQ(clientComponent.getProperty("Ratio").getDefaultValue(), Ratio(1, 1000));

    // A server-side update re-syncs the client via remoteUpdate
    updateHelper(serverComponent, serializeHelper(serverComponent));

    const auto clientProp = clientComponent.getProperty("Ratio");
    ASSERT_EQ(clientProp.getDefaultValue(), Ratio(1, 10));
    ASSERT_EQ(clientProp.getDescription(), "changed");
    ASSERT_EQ(clientComponent.getPropertyValue("Ratio"), Ratio(1, 10));

    // The property order matches the server after the replacement
    auto serverNames = List<IString>();
    for (const auto& prop : serverComponent.getAllProperties())
        serverNames.pushBack(prop.getName());
    auto clientNames = List<IString>();
    for (const auto& prop : clientComponent.getAllProperties())
        clientNames.pushBack(prop.getName());
    ASSERT_EQ(serverNames, clientNames);
}

TEST_F(ConfigRemoteUpdateTest, UpdateKeepsUnchangedProperties)
{
    auto serverComponent = serverDevice.getCustomComponents()[0];
    auto clientComponent = clientDevice.getCustomComponents()[0];

    const auto clientPropBefore = clientComponent.getProperty("Ratio");
    updateHelper(serverComponent, serializeHelper(serverComponent));

    // Unchanged properties are not replaced
    ASSERT_EQ(clientComponent.getProperty("Ratio").getObject(), clientPropBefore.getObject());
}

TEST_F(ConfigRemoteUpdateTest, UpdateChannelActive)
{
    auto serverChannel = serverDevice.getChannelsRecursive()[0];
    auto clientChannel = clientDevice.getChannelsRecursive()[0];

    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
    const auto str = serializeHelper(serverChannel);
    
    serverChannel.setActive(false);

    ASSERT_FALSE(serverChannel.getActive());
    ASSERT_FALSE(clientChannel.getActive());

    updateHelper(serverChannel, str);
    
    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
}

TEST_F(ConfigRemoteUpdateTest, UpdateHierarchicalActive)
{
    auto serverChannel = serverDevice.getChannelsRecursive()[0];
    auto clientChannel = clientDevice.getChannelsRecursive()[0];

    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
    const auto str = serializeHelper(serverDevice);

    serverDevice.setActive(false);

    ASSERT_FALSE(serverChannel.getActive());
    ASSERT_FALSE(clientChannel.getActive());

    updateHelper(serverDevice, str);

    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
}

// Rewrites the escaped JSON of a ComponentUpdateEnd notification so every serialized device looks
// like one sent by a server older than config protocol version 25: "DaqDeviceInfo" is removed from
// "propValues" and from "properties", and the device info object is placed under the dedicated
// legacy "deviceInfo" key of the device instead. Braces are matched textually; the mock device
// tree has no braces inside string values.
static std::string toLegacyDeviceInfoFormat(std::string json, size_t& rewritten)
{
    const std::string key = R"(\"DaqDeviceInfo\":)";
    const std::string propValuesKey = R"(\"propValues\":)";
    rewritten = 0;

    const auto findMatchingClose = [&json](size_t open)
    {
        size_t depth = 0;
        size_t close = open;
        for (; close < json.size(); ++close)
        {
            if (json[close] == '{')
                ++depth;
            else if (json[close] == '}' && --depth == 0)
                break;
        }
        return close;
    };

    // The first unmatched '{' before "pos"
    const auto findEnclosingOpen = [&json](size_t pos)
    {
        int balance = 0;
        size_t open = pos;
        while (open > 0)
        {
            --open;
            if (json[open] == '}')
                ++balance;
            else if (json[open] == '{')
            {
                if (balance == 0)
                    break;
                --balance;
            }
        }
        return open;
    };

    const auto eraseWithComma = [&json](size_t begin, size_t end)
    {
        if (end < json.size() && json[end] == ',')
            ++end;
        else if (begin > 0 && json[begin - 1] == ',')
            --begin;
        json.erase(begin, end - begin);
        return begin;
    };

    size_t pos = 0;
    while ((pos = json.find(key, pos)) != std::string::npos)
    {
        const size_t objStart = pos + key.size();
        if (json[objStart] != '{')
            throw std::runtime_error("DaqDeviceInfo value is not an object");

        const size_t objEnd = findMatchingClose(objStart);
        const std::string infoObj = json.substr(objStart, objEnd - objStart + 1);
        const size_t erasedAt = eraseWithComma(pos, objEnd + 1);

        const size_t open = findEnclosingOpen(erasedAt);
        if (open < propValuesKey.size() || json.compare(open - propValuesKey.size(), propValuesKey.size(), propValuesKey) != 0)
            throw std::runtime_error("DaqDeviceInfo is not a direct entry of propValues");

        const std::string legacyEntry = R"(\"deviceInfo\":)" + infoObj + ",";
        const size_t insertAt = open - propValuesKey.size();
        json.insert(insertAt, legacyEntry);
        pos = insertAt + legacyEntry.size();
        ++rewritten;
    }

    const std::string propEntryKey = R"(\"name\":\"DaqDeviceInfo\")";
    pos = 0;
    while ((pos = json.find(propEntryKey, pos)) != std::string::npos)
    {
        const size_t open = findEnclosingOpen(pos);
        const size_t close = findMatchingClose(open);
        pos = eraseWithComma(open, close + 1);
    }
    return json;
}

static size_t countDevices(const DevicePtr& device)
{
    size_t count = 1;
    for (const auto& sub : device.getDevices())
        count += countDevices(sub);
    return count;
}

// Regression: a server older than protocol version 25 does not serialize DaqDeviceInfo as a
// property value. When the client mirror rebuilds itself from ComponentUpdateEnd it clears the
// property locally; that clear must not turn into a ClearProtectedPropertyValue request, because
// it is issued from the notification handler and could never be answered by the native client.
TEST_F(ConfigRemoteUpdateTest, UpdateFromServerWithoutDaqDeviceInfoSendsNoRequests)
{
    const auto serverInfo = serverDevice.getInfo();
    const auto serverSubInfo = serverDevice.getDevices()[0].getInfo();
    ASSERT_EQ(serverInfo.getName(), "root_dev");
    ASSERT_EQ(serverInfo.getLocation(), "loc");
    ASSERT_FALSE(serverSubInfo.getName().toStdString().empty());

    size_t rewrittenDevices = 0;
    std::string rewriteError;
    notificationRewriter = [&rewrittenDevices, &rewriteError](const PacketBuffer& packet)
    {
        const auto original = packet.parseServerNotification();
        try
        {
            size_t rewritten = 0;
            const std::string json = toLegacyDeviceInfoFormat(original.toStdString(), rewritten);
            rewrittenDevices += rewritten;
            return PacketBuffer::createServerNotification(json.c_str(), json.size());
        }
        catch (const std::exception& e)
        {
            // Must not throw into the server's notification callback
            rewriteError = e.what();
            return PacketBuffer::createServerNotification(original.getCharPtr(), original.getLength());
        }
    };

    rpcRequests.clear();
    updateHelper(clientDevice, serializeHelper(referenceDevice));

    ASSERT_TRUE(rewriteError.empty()) << rewriteError;
    ASSERT_EQ(rewrittenDevices, countDevices(serverDevice));

    // Nothing may be requested from the server while the notification is being handled
    ASSERT_TRUE(requestsDuringNotification.empty()) << requestsDuringNotification.front();
    for (const auto& request : rpcRequests)
        ASSERT_EQ(request.find("ClearProtectedPropertyValue"), std::string::npos) << request;

    // Device info of the mirrors is taken from the legacy key and keeps the server values
    ASSERT_EQ(clientDevice.getInfo().getName(), serverInfo.getName());
    ASSERT_EQ(clientDevice.getInfo().getLocation(), serverInfo.getLocation());
    ASSERT_EQ(clientDevice.getDevices()[0].getInfo().getName(), serverSubInfo.getName());
    ASSERT_EQ(clientDevice.getDevices()[0].getInfo().getManufacturer(), serverSubInfo.getManufacturer());
}
