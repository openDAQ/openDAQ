#include <gtest/gtest.h>
#include <opendaq/device_impl.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace daq;
using namespace daq::config_protocol;

// Not an anonymous namespace: unity builds can merge this file with other test files
namespace config_protocol_singleton_test
{

// Offers a singleton function block type, as a device does whose module allows one instance of a block per parent
class SingletonDeviceImpl final : public Device
{
public:
    // lockedRemove false removes without holding the config lock over the removal and its core event
    SingletonDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, bool singleton, bool lockedRemove = true)
        : Device(ctx, parent, localId)
        , singleton(singleton)
        , lockedRemove(lockedRemove)
    {
    }

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        const FunctionBlockTypePtr singleType = FunctionBlockTypeBuilder().setId("Single").setName("Single").setSingleton(singleton).build();
        const FunctionBlockTypePtr optionsType = FunctionBlockTypeBuilder()
                                                     .setId("Options")
                                                     .setName("Options")
                                                     .setInputPolicy(FunctionBlockInputPolicy::SettingsDriven)
                                                     .setCommonSettingsTypeId("Single")
                                                     .build();
        return Dict<IString, IFunctionBlockType>(
            {{"Single", singleType}, {"Multi", FunctionBlockType("Multi", "Multi", "")}, {"Options", optionsType}});
    }

    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override
    {
        const FunctionBlockTypePtr type = onGetAvailableFunctionBlockTypes().get(typeId);

        // Loading a configuration passes the saved local ID
        StringPtr localId = typeId.toStdString() + "_" + std::to_string(nextId++);
        if (config.assigned() && config.hasProperty("LocalId"))
            localId = config.getPropertyValue("LocalId");

        auto lock = this->getRecursiveConfigLock2();
        const auto fb = createWithImplementation<IFunctionBlock, FunctionBlock>(type, this->context, this->functionBlocks, localId);
        addNestedFunctionBlock(fb);
        return fb;
    }

    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override
    {
        if (!lockedRemove)
        {
            removeNestedFunctionBlock(functionBlock);
            return;
        }

        auto lock = this->getRecursiveConfigLock2();
        removeNestedFunctionBlock(functionBlock);
    }

private:
    bool singleton;
    bool lockedRemove;
    std::atomic<int> nextId = 0;
};

DevicePtr createSingletonDevice(bool singleton)
{
    return createWithImplementation<IDevice, SingletonDeviceImpl>(NullContext(), nullptr, "dev", singleton);
}

std::vector<std::string> functionBlockIds(const DevicePtr& device)
{
    std::vector<std::string> ids;
    for (const auto& fb : device.getFunctionBlocks())
        ids.push_back(fb.getLocalId());
    return ids;
}

}

using namespace config_protocol_singleton_test;

class ConfigProtocolSingletonTest : public testing::Test
{
public:
    void SetUp() override
    {
        connect(true);
    }

protected:
    void connect(bool lockedRemove)
    {
        const auto serverContext = NullContext();
        serverDevice = createWithImplementation<IDevice, SingletonDeviceImpl>(serverContext, nullptr, "dev", true, lockedRemove);

        // Subscribed before the server, so it can hold a removal event back before the server sends it to the client
        serverContext.getOnCoreEvent() += [this](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            if (args.getEventId() == static_cast<Int>(CoreEventId::ComponentRemoved) && holdRemovalEvent.exchange(false))
            {
                removalEventHeld.set_value();
                additionDone.get_future().wait_for(std::chrono::seconds(1));
            }
        };

        // The client mirrors a configuration loaded on the server from the core events the server sends
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(serverDevice,
                                                        [this](const PacketBuffer& packet)
                                                        {
                                                            if (deliverNotifications)
                                                                client->triggerNotificationPacket(packet);
                                                        },
                                                        User("", ""),
                                                        ClientType::Control,
                                                        test_utils::dummyExtSigFolder(serverDevice.getContext()));

        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            NullContext(),
            [this](const PacketBuffer& packet) { return server->processRequestAndGetReply(packet); },
            [this](const PacketBuffer& packet) { server->processNoReplyRequest(packet); },
            nullptr,
            nullptr,
            nullptr);
        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    // false drops the server's notifications, so the client does not see what changed on the server
    std::atomic_bool deliverNotifications{true};

    std::atomic_bool holdRemovalEvent{false};
    std::promise<void> removalEventHeld;
    std::promise<void> additionDone;

    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
};

TEST_F(ConfigProtocolSingletonTest, AddSecondSingletonThroughClientFails)
{
    clientDevice.addFunctionBlock("Single");

    // The client's copy holds the instance, so the client refuses the addition itself
    ASSERT_THROW(clientDevice.addFunctionBlock("Single"), AlreadyExistsException);
    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_0"}));
    ASSERT_EQ(functionBlockIds(clientDevice), (std::vector<std::string>{"Single_0"}));
}

// The client does not know about the server's instance, so its own check passes and the server refuses the addition
TEST_F(ConfigProtocolSingletonTest, ServerRefusalReachesClient)
{
    deliverNotifications = false;
    serverDevice.addFunctionBlock("Single");
    ASSERT_EQ(clientDevice.getFunctionBlocks().getCount(), 0u);

    daq::FunctionBlockPtr fb;
    ASSERT_EQ(clientDevice->addFunctionBlock(&fb, String("Single"), nullptr), OPENDAQ_ERR_ALREADYEXISTS);
    daqClearErrorInfo();
    ASSERT_THROW(clientDevice.addFunctionBlock("Single"), AlreadyExistsException);

    // The refused addition leaves the client's copy and the server unchanged
    ASSERT_EQ(clientDevice.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_0"}));
}

TEST_F(ConfigProtocolSingletonTest, LoadConfigurationThroughClientRestoresSingleton)
{
    clientDevice.addFunctionBlock("Single");
    clientDevice.addFunctionBlock("Multi");
    const auto configuration = serverDevice.saveConfiguration();

    // The loaded instance replaces the existing one instead of being refused as a second instance
    ASSERT_NO_THROW(clientDevice.loadConfiguration(configuration));
    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_0", "Multi_1"}));
    ASSERT_EQ(functionBlockIds(clientDevice), (std::vector<std::string>{"Single_0", "Multi_1"}));
}

TEST_F(ConfigProtocolSingletonTest, LoadConfigurationThroughClientSkipsDuplicateSingleton)
{
    const auto source = createSingletonDevice(false);
    source.addFunctionBlock("Single");
    source.addFunctionBlock("Single");
    source.addFunctionBlock("Multi");
    const auto configuration = source.saveConfiguration();

    // The client forwards the setup to the server, which skips the second instance and still loads the rest of
    // the setup. The client then mirrors the result.
    ASSERT_NO_THROW(clientDevice.loadConfiguration(configuration));
    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_0", "Multi_2"}));
    ASSERT_EQ(functionBlockIds(clientDevice), (std::vector<std::string>{"Single_0", "Multi_2"}));
}

TEST_F(ConfigProtocolSingletonTest, ClientFunctionBlocksHaveTypeOptions)
{
    clientDevice.addFunctionBlock("Single");
    clientDevice.addFunctionBlock("Options");
    clientDevice.addFunctionBlock("Multi");

    const auto clientFbs = clientDevice.getFunctionBlocks();
    ASSERT_EQ(clientFbs.getCount(), 3u);

    const auto singleType = clientFbs[0].getFunctionBlockType();
    ASSERT_EQ(singleType.getId(), "Single");
    ASSERT_TRUE(singleType.getSingleton());

    const auto optionsType = clientFbs[1].getFunctionBlockType();
    ASSERT_EQ(optionsType.getId(), "Options");
    ASSERT_EQ(optionsType.getInputPolicy(), FunctionBlockInputPolicy::SettingsDriven);
    ASSERT_FALSE(optionsType.getSingleton());
    ASSERT_EQ(optionsType.getCommonSettingsTypeId(), "Single");

    const auto multiType = clientFbs[2].getFunctionBlockType();
    ASSERT_EQ(multiType.getInputPolicy(), FunctionBlockInputPolicy::Unspecified);
    ASSERT_FALSE(multiType.getSingleton());
    ASSERT_FALSE(multiType.getCommonSettingsTypeId().assigned());
}

// A client that connects after the function blocks were added gets the options from the initial structure
TEST_F(ConfigProtocolSingletonTest, ClientConnectingLaterHasTypeOptions)
{
    serverDevice.addFunctionBlock("Single");

    auto lateClient = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
        NullContext(),
        [this](const PacketBuffer& packet) { return server->processRequestAndGetReply(packet); },
        [this](const PacketBuffer& packet) { server->processNoReplyRequest(packet); },
        nullptr,
        nullptr,
        nullptr);
    const auto lateClientDevice = lateClient->connect();

    ASSERT_TRUE(lateClientDevice.getFunctionBlocks()[0].getFunctionBlockType().getSingleton());
}

// Replaces the singleton instance on the server while the removal's core event is held back, so the server sends
// the addition of the new instance before the removal of the old one
class ConfigProtocolSingletonOrderTest : public ConfigProtocolSingletonTest
{
public:
    void SetUp() override
    {
    }

protected:
    void replaceSingletonWithRemovalEventHeldBack()
    {
        clientDevice.addFunctionBlock("Single");

        holdRemovalEvent = true;
        std::thread remover([this] { serverDevice.removeFunctionBlock(serverDevice.getFunctionBlocks()[0]); });

        // The folder no longer holds the old instance, the client has not been told yet
        removalEventHeld.get_future().wait();
        serverDevice.addFunctionBlock("Single");
        additionDone.set_value();

        remover.join();
    }
};

TEST_F(ConfigProtocolSingletonOrderTest, ClientMirrorsReplacementWithEventsOutOfOrder)
{
    connect(false);
    replaceSingletonWithRemovalEventHeldBack();

    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_1"}));
    ASSERT_EQ(functionBlockIds(clientDevice), functionBlockIds(serverDevice));
}

TEST_F(ConfigProtocolSingletonOrderTest, ClientMirrorsReplacementWhenRemovalHoldsLock)
{
    connect(true);
    replaceSingletonWithRemovalEventHeldBack();

    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_1"}));
    ASSERT_EQ(functionBlockIds(clientDevice), functionBlockIds(serverDevice));
}
