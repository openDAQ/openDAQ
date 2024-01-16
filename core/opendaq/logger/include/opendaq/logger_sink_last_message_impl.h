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
#include <spdlog/sinks/base_sink.h>
#include <mutex>

namespace spdlog {
namespace sinks {

// TODO: add logger sink private
template<typename Mutex>
class LoggerSinkLastMessage : public base_sink<Mutex>
{
protected:
    // base_sink();
    // explicit base_sink(std::unique_ptr<spdlog::formatter> formatter);

    // set_level(level::level_enum::debug);
    void sink_it_(const details::log_msg& msg) override
    {
        std::lock_guard lock(mx);
        lastMessage = fmt::to_string(msg.payload);
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

private:
    std::mutex mx;
    daq::StringPtr lastMessage;
};

using LoggerSinkLastMessageMt = LoggerSinkLastMessage<std::mutex>;
using LoggerSinkLastMessageSt = LoggerSinkLastMessage<spdlog::details::null_mutex>;

} // namespace sinks
} // namespace spdlog