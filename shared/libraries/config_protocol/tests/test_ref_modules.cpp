// ReSharper disable CppClangTidyModernizeAvoidBind
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/context_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/instance_factory.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;

class ConfigProtocolRefModulesTest : public Test
{
public:
    void SetUp() override
    {
        instance = Instance();
        // ReSharper disable once CppExpressionWithoutSideEffects
        instance.addDevice("daqref://device0");

        server = std::make_unique<ConfigProtocolServer>(instance.getRootDevice(), nullptr, anonymousUser);

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext, [this](const PacketBuffer& requestPacket) -> PacketBuffer
            {
                return server->processRequestAndGetReply(requestPacket);
            }, nullptr);
    }


protected:
    InstancePtr instance;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    ContextPtr clientContext;
    const UserPtr anonymousUser = User("", "");

};

#if defined(OPENDAQ_TEST_WITH_REF_MODULES)

TEST_F(ConfigProtocolRefModulesTest, Test)
{
    const auto instance = Instance();
    // ReSharper disable once CppExpressionWithoutSideEffects
    instance.setRootDevice("daqref://device0");
    ConfigProtocolServer server(instance, nullptr, anonymousUser);

    clientContext = NullContext();
    ConfigProtocolClient<ConfigClientDeviceImpl> client(
        clientContext,
        [&server](const PacketBuffer& requestPacket) -> PacketBuffer
        {
            return server.processRequestAndGetReply(requestPacket);
        },
        nullptr);

    const auto clientDevice = client.connect();
    const auto ch = clientDevice.getChannels()[0];
    ch.setPropertyValue("Amplitude", 0.2);

    const auto clientSignal = ch.getSignals()[0];

    const auto clientFunctionBlock = clientDevice.addFunctionBlock("RefFBModuleStatistics");
    const auto clientInputPort = clientFunctionBlock.getInputPorts()[0];

    clientInputPort.connect(clientSignal);
}

#endif
