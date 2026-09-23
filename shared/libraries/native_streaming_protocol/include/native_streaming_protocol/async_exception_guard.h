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

#pragma once
#include <native_streaming_protocol/native_streaming_protocol.h>
#include <opendaq/custom_log.h>

#include <boost/asio/io_context.hpp>

#include <string_view>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

/*!
 * @brief Invokes a callable, logging any exception it throws instead of letting it escape.
 *
 * Intended for callables invoked on a thread that cannot handle a failure itself - a completion
 * handler, a thread function, a callback crossing a C boundary - where an escaping exception is
 * an unhandled exception in that thread and terminates the whole process.
 *
 * The description is given as a format string plus its arguments rather than a ready-made string,
 * so that nothing is formatted or allocated unless the callable actually throws. This matters on
 * call sites invoked per request.
 *
 * @param loggerComponent The logger component to report the failure to.
 * @param callable The callable to invoke.
 * @param descriptionFormat Describes the operation, used as the log message prefix.
 * @param descriptionArgs Arguments for @p descriptionFormat, formatted only on failure.
 * @returns True if the callable ran to completion, false if it threw.
 */
template <typename Callable, typename... DescriptionArgs>
bool runGuarded(const LoggerComponentPtr& loggerComponent,
                const Callable& callable,
                fmt::format_string<DescriptionArgs...> descriptionFormat,
                DescriptionArgs&&... descriptionArgs)
{
    try
    {
        callable();
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_E("{}: unhandled exception: {}",
              fmt::format(descriptionFormat, std::forward<DescriptionArgs>(descriptionArgs)...),
              e.what());
    }
    catch (...)
    {
        LOG_E("{}: unhandled exception", fmt::format(descriptionFormat, std::forward<DescriptionArgs>(descriptionArgs)...));
    }
    return false;
}

/*!
 * @brief Runs a Boost.Asio event loop, resuming it whenever a completion handler throws.
 *
 * Boost.Asio propagates an exception thrown by a handler out of @c io_context::run(), which is
 * invoked directly as the body of a thread function - so an escaping handler exception terminates
 * the process. Resuming matters as much as catching: if the loop was left after a handler threw,
 * the handlers still queued would never run and stopping the owner of the context would block
 * forever.
 *
 * @param ioContext The IO context to run.
 * @param loggerComponent The logger component to report handler failures to.
 * @param description Describes the thread, used as the log message prefix.
 */
inline void runGuardedEventLoop(boost::asio::io_context& ioContext,
                                const LoggerComponentPtr& loggerComponent,
                                std::string_view description)
{
    while (!runGuarded(
        loggerComponent, 
        [&ioContext]()
        {
            ioContext.run();
        }, 
        "{}",
        description)
    );
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
