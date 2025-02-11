/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#pragma once
#include <opendaq/opendaq.h>
#include <thread>
#include "testutils/memcheck_listener.h"

// MAC CI issue
#if !defined(SKIP_TEST_MAC_CI)
    #if defined(__clang__)
        #define SKIP_TEST_MAC_CI GTEST_SKIP() << "Skipping test on MacOs"
    #else
        #define SKIP_TEST_MAC_CI
    #endif
#endif

BEGIN_NAMESPACE_OPENDAQ

namespace docs_test_helpers
{
    [[maybe_unused]]
    static InstancePtr setupSimulatorServers()
    {
        auto instance = Instance();
        
        instance.setRootDevice("daq.template://device1");
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING) && defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        instance.addServer("OpenDAQLTStreaming", nullptr);
#endif
        instance.addStandardServers();
        return instance;
    }

    [[maybe_unused]]
    static InstancePtr setupInstance()
    {
        auto instance = Instance();
        
        instance.setRootDevice("daqref://device1");
        return instance;
    }

    // On windows CI runner delay >50ms is needed for samples to be received via streaming
    // and be ready to read
    [[maybe_unused]]
    static void waitForSamplesReady()
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
    }
}

END_NAMESPACE_OPENDAQ
