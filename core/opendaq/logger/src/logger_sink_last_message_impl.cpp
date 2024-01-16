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

#include <opendaq/logger_sink_last_message_impl.h>
#include <coretypes/validation.h>

namespace spdlog {
namespace sinks {

// template<typename Mutex>
// void LoggerSinkLastMessage<Mutex>::sink_it_(const details::log_msg& msg)
// {
//     std::lock_guard lock(mx);
//     lastMessage = fmt::to_string(msg.payload);
// }

// template<typename Mutex>
// void LoggerSinkLastMessage<Mutex>::flush_()
// {
// }

// template<typename Mutex>
// daq::ErrCode LoggerSinkLastMessage<Mutex>::getLastMessage(daq::IString** lastMessage)
// {
//     OPENDAQ_PARAM_NOT_NULL(lastMessage);

//     std::lock_guard lock(mx);
//     *lastMessage = this->lastMessage.addRefAndReturn();
//     return OPENDAQ_SUCCESS;
// }

} // namespace sinks
} // namespace spdlog