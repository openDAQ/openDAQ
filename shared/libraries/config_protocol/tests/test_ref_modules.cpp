// ReSharper disable CppClangTidyModernizeAvoidBind
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/device_ptr.h>
#include <opendaq/instance_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/context_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/instance_factory.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>

#if defined(OPENDAQ_TEST_WITH_REF_MODULES)
    #include <ref_fb_module/module_dll.h>
    #include <ref_device_module/module_dll.h>
#endif

using namespace daq;
using namespace config_protocol;
using namespace testing;

using ConfigProtocolRefModulesTest = testing::Test;

#if defined(OPENDAQ_TEST_WITH_REF_MODULES)

TEST_F(ConfigProtocolRefModulesTest, Test)
{
    InstancePtr instance = Instance("[[none]]");
    {
        ContextPtr context = instance.getContext();

        ModulePtr refFbs;
        createRefFBModule(&refFbs, context);

        ModulePtr refDev;
        createRefDeviceModule(&refDev, context);

        auto mm = instance.getModuleManager();
        mm.addModule(refFbs);
        mm.addModule(refDev);
    }

    // ReSharper disable once CppExpressionWithoutSideEffects
    instance.setRootDevice("daqref://device0");
    ConfigProtocolServer server(instance, nullptr, User("", ""), ClientType::Control, test_utils::dummyExtSigFolder(instance.getContext()));

    auto clientContext = NullContext();
    ConfigProtocolClient<ConfigClientDeviceImpl> client(
        clientContext,
        [&server](const PacketBuffer& requestPacket) -> PacketBuffer
        {
            return server.processRequestAndGetReply(requestPacket);
        },
        [&server](const PacketBuffer& requestPacket)
        {
            // callback is not expected to be called within this test group
            assert(false);
            server.processNoReplyRequest(requestPacket);
        },
        nullptr,
        nullptr,
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
