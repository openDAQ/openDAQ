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

        server = std::make_unique<ConfigProtocolServer>(instance.getRootDevice(), nullptr);

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient>(
            clientContext, [this](const PacketBuffer& requestPacket) -> PacketBuffer
            {
                return server->processRequestAndGetReply(requestPacket);
            }, nullptr);
    }


protected:
    InstancePtr instance;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient> client;
    ContextPtr clientContext;

};

TEST_F(ConfigProtocolRefModulesTest, Test)
{
    const auto instance = Instance();
    // ReSharper disable once CppExpressionWithoutSideEffects
    instance.setRootDevice("daqref://device0");
    ConfigProtocolServer server(instance, nullptr);

    clientContext = NullContext();
    ConfigProtocolClient client(
        clientContext,
        [&server](const PacketBuffer& requestPacket) -> PacketBuffer
        {
            return server.processRequestAndGetReply(requestPacket);
        },
        nullptr);


    client.connect();

    const auto clientDevice = client.getDevice();
    const auto ch = clientDevice.getChannels()[0];
    ch.setPropertyValue("Amplitude", 0.2);

    const auto clientSignal = ch.getSignals()[0];

    const auto clientFunctionBlock = clientDevice.addFunctionBlock("ref_fb_module_statistics");
    const auto clientInputPort = clientFunctionBlock.getInputPorts()[0];

    clientInputPort.connect(clientSignal);
}
