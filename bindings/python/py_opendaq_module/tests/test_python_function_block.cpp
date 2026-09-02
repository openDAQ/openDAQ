/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <py_opendaq_module/python_module.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_factory.h>

#include <chrono>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

class PythonFunctionBlockTest : public testing::Test
{
protected:
    static ModulePtr loadMockModule()
    {
        return createPythonModule(NullContext(), std::string(MOCK_MODULE_DIR) + "/mock_module_fb.py");
    }

    // Async hooks (onConnected/onDisconnected/onPacketReceived) run on PythonRuntime's dispatch
    // thread after the triggering C++ call already returned, so observing their effect means
    // polling with a bound instead of asserting immediately.
    template <typename Predicate>
    static bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout = 2s)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (predicate())
                return true;
            std::this_thread::sleep_for(5ms);
        }
        return predicate();
    }
};

TEST_F(PythonFunctionBlockTest, CreatesFunctionBlockWithCorrectType)
{
    const auto module = loadMockModule();
    const auto fb = module.createFunctionBlock("mock_fb", nullptr, "fb");

    ASSERT_TRUE(fb.assigned());
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "MockFunctionBlock");
}

TEST_F(PythonFunctionBlockTest, OnInitAddsSignalAndInputPortThroughCppFb)
{
    const auto module = loadMockModule();
    const auto fb = module.createFunctionBlock("mock_fb", nullptr, "fb");

    const auto signals = fb.getSignals();
    ASSERT_EQ(signals.getCount(), 1u);
    ASSERT_EQ(signals[0].getLocalId(), "output");

    const auto inputPorts = fb.getInputPorts();
    ASSERT_EQ(inputPorts.getCount(), 1u);
    ASSERT_EQ(inputPorts[0].getLocalId(), "input");
}

TEST_F(PythonFunctionBlockTest, OnInitSetsStatusMessageThroughCppFb)
{
    const auto module = loadMockModule();
    const auto fb = module.createFunctionBlock("mock_fb", nullptr, "fb");

    const auto container = fb.getStatusContainer();
    ASSERT_EQ(container.getStatus("ComponentStatus").getValue(), "Warning");
    ASSERT_EQ(container.getStatusMessage("ComponentStatus"), "mock warning");
}

TEST_F(PythonFunctionBlockTest, GetStatusSignalReturnsWhatOnGetStatusSignalReturns)
{
    const auto module = loadMockModule();
    const auto fb = module.createFunctionBlock("mock_fb", nullptr, "fb");

    const auto statusSignal = fb.getStatusSignal();
    ASSERT_TRUE(statusSignal.assigned());
    ASSERT_EQ(statusSignal.getLocalId(), "status");
}

TEST_F(PythonFunctionBlockTest, OnConnectedIsDispatchedAsynchronously)
{
    const auto module = loadMockModule();
    const auto fb = module.createFunctionBlock("mock_fb", nullptr, "fb");
    ASSERT_EQ(fb.getSignals().getCount(), 1u);

    const auto externalSignal = Signal(NullContext(), nullptr, "external");
    fb.getInputPorts()[0].connect(externalSignal);

    const bool gotSecondSignal = waitUntil([&] { return fb.getSignals().getCount() == 2u; });
    ASSERT_TRUE(gotSecondSignal);

    const auto signals = fb.getSignals();
    ASSERT_TRUE(signals[0].getLocalId() == "connected_marker" || signals[1].getLocalId() == "connected_marker");
}

TEST_F(PythonFunctionBlockTest, UnsupportedIdThrowsNotFound)
{
    const auto module = loadMockModule();
    ASSERT_THROW(module.createFunctionBlock("not_mock_fb", nullptr, "fb"), NotFoundException);
}
