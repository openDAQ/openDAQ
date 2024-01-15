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

#include <coretypes/string_ptr.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

BEGIN_NAMESPACE_OPENDAQ

// TODO: add logger sink private
template<typename Mutex>
class LastMessageSink : public spdlog::sinks::base_sink<Mutex>
{
    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override;
        void flush_() override;

    public:
        // TODO: override from logger sink private
        ErrCode getLastMessage(IString** lastMessage);

    private:
        StringPtr lastMessage;
};

END_NAMESPACE_OPENDAQ