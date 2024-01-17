/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace spdlog {
namespace sinks {

// TODO: add logger sink private
template<typename Mutex>
class LoggerSinkLastMessage : public base_sink<Mutex>
{
protected:
    void sink_it_(const details::log_msg& msg) override
    {
        {
            std::lock_guard lock(mx);
            if (finishing) return;
            lastMessage = fmt::to_string(msg.payload);
            newMessage = true;
        }
        cv.notify_all();
    }

    void flush_() override
    {
    }

public:
    daq::ErrCode getLastMessage(daq::IString** lastMessage)
    {
        OPENDAQ_PARAM_NOT_NULL(lastMessage);

        std::lock_guard lock(mx);
        *lastMessage = this->lastMessage.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode waitForMessage(daq::SizeT timeoutMs, daq::Bool* success)
    {
        std::unique_lock lock(mx);
        if (finishing) return false;
        auto result = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]{ return newMessage; });
        newMessage = false;
        return result;
    }

    ~LoggerSinkLastMessage() 
    {
        {
            std::lock_guard lock(mx);
            finishing = true;
            newMessage = true;
        }
        cv.notify_all();
    }

private:
    std::mutex mx;
    std::condition_variable cv;
    bool newMessage = false;
    bool finishing = false;
    daq::StringPtr lastMessage;
};

using LoggerSinkLastMessageMt = LoggerSinkLastMessage<std::mutex>;
using LoggerSinkLastMessageSt = LoggerSinkLastMessage<spdlog::details::null_mutex>;

} // namespace sinks
} // namespace spdlog