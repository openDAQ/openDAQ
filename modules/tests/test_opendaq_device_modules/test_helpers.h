/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <thread>
#include <future>

#include <opendaq/opendaq.h>
#include <testutils/testutils.h>
#include "testutils/memcheck_listener.h"

// MAC CI issue
#if !defined(SKIP_TEST_MAC_CI)
#   if defined(__clang__)
#       define SKIP_TEST_MAC_CI return
#   else
#       define SKIP_TEST_MAC_CI
#   endif
#endif

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{
    [[maybe_unused]]
    static void setupSubscribeAckHandler(
        std::promise<StringPtr>& acknowledgementPromise,
        std::future<StringPtr>& acknowledgementFuture,
        MirroredSignalConfigPtr& signal
    )
    {
        acknowledgementFuture = acknowledgementPromise.get_future();
        auto signalId = std::make_shared<std::string>(signal.getGlobalId().toStdString());
        signal.getOnSubscribeComplete() +=
            [&acknowledgementPromise, &acknowledgementFuture, signalId]
            (MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args)
        {
            if (acknowledgementFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            {
                acknowledgementPromise.set_value(args.getStreamingConnectionString());
            }
            else
            {
                ADD_FAILURE()  << " Set already satisfied subscribe ack promise for signal: "
                               << *signalId.get()
                               << "\n streaming: "
                               << args.getStreamingConnectionString();
            }
        };
    }

    [[maybe_unused]]
    static void setupUnsubscribeAckHandler(
        std::promise<StringPtr>& acknowledgementPromise,
        std::future<StringPtr>& acknowledgementFuture,
        MirroredSignalConfigPtr& signal
    )
    {
        acknowledgementFuture = acknowledgementPromise.get_future();
        auto signalId = std::make_shared<std::string>(signal.getGlobalId().toStdString());
        signal.getOnUnsubscribeComplete() +=
            [&acknowledgementPromise, &acknowledgementFuture, signalId]
            (MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args)
        {
            if (acknowledgementFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            {
                acknowledgementPromise.set_value(args.getStreamingConnectionString());
            }
            else
            {
                ADD_FAILURE()  << " Set already satisfied unsubscribe ack promise for signal: "
                              << *signalId.get()
                              << "\n streaming: "
                              << args.getStreamingConnectionString();
            }
        };
    }

    [[maybe_unused]]
    static bool waitForAcknowledgement(
        std::future<StringPtr>& acknowledgementFuture,
        std::chrono::seconds timeout = std::chrono::seconds(5))
    {
        return acknowledgementFuture.wait_for(timeout) == std::future_status::ready;
    }
}

END_NAMESPACE_OPENDAQ
