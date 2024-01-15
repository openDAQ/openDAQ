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

BEGIN_NAMESPACE_OPENDAQ

template<typename Mutex>
void LastMessageSink<Mutex>::sink_it_(const spdlog::details::log_msg& msg)
{
    lastMessage = fmt::to_string(msg.payload);
}

template<typename Mutex>
void LastMessageSink<Mutex>::flush_()
{
}

template<typename Mutex>
ErrCode LastMessageSink<Mutex>::getLastMessage(IString** lastMessage)
{
    OPENDAQ_PARAM_NOT_NULL(lastMessage);

    *lastMessage = this->lastMessage.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ