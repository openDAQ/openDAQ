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
#include <string>
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
    SingletonDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, bool validatesAdd)
        : Device(ctx, parent, localId)
        , validatesAdd(validatesAdd)
    {
    }

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        const FunctionBlockTypePtr singleType = FunctionBlockTypeBuilder().setId("Single").setName("Single").setSingleton(True).build();
        return Dict<IString, IFunctionBlockType>({{"Single", singleType}, {"Multi", FunctionBlockType("Multi", "Multi", "")}});
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
        auto lock = this->getRecursiveConfigLock2();
        removeNestedFunctionBlock(functionBlock);
    }

protected:
    // false saves setups with duplicates, as a parent that does not enforce the singleton flag would
    bool validatesAddFunctionBlock() override
    {
        return validatesAdd;
    }

private:
    bool validatesAdd;
    std::atomic<int> nextId = 0;
};

DevicePtr createSingletonDevice(bool validatesAdd)
{
    return createWithImplementation<IDevice, SingletonDeviceImpl>(NullContext(), nullptr, "dev", validatesAdd);
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
        serverDevice = createSingletonDevice(true);
        // The client mirrors a configuration loaded on the server from the core events the server sends
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(serverDevice,
                                                        [this](const PacketBuffer& packet) { client->triggerNotificationPacket(packet); },
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

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
};

TEST_F(ConfigProtocolSingletonTest, AddSecondSingletonThroughClientFails)
{
    clientDevice.addFunctionBlock("Single");

    // The server refuses the addition and the client gets its error code
    ASSERT_THROW(clientDevice.addFunctionBlock("Single"), AlreadyExistsException);
    ASSERT_EQ(functionBlockIds(serverDevice), (std::vector<std::string>{"Single_0"}));
    ASSERT_EQ(functionBlockIds(clientDevice), (std::vector<std::string>{"Single_0"}));
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
