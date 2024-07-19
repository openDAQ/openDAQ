/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/coretypes.h>
#include <coretypes/event_handler_ptr.h>
#include <coretypes/utility_sync.h>
#include <vector>

namespace std
{
    template <>
    struct hash<daq::EventHandler>
    {
        using argument_type = daq::EventHandler;
        using result_type = std::size_t;

        result_type operator()(argument_type const& s) const noexcept
        {
            if (!s.assigned())
            {
                return 0;
            }

            return s.getHashCode();
        }
    };

    template <>
    struct equal_to<daq::EventHandler>
    {
        using first_argument_type = daq::EventHandler;
        using second_argument_type = daq::EventHandler;
        using result_type = bool;

        result_type operator()(first_argument_type const& lhs, second_argument_type const& rhs) const noexcept
        {
            return lhs.getHashCode() == rhs.getHashCode();
        }
    };
}

BEGIN_NAMESPACE_OPENDAQ

class EventImpl : public ImplementationOf<IEvent, IFreezable>
{
    struct Handler
    {
        EventHandler eventHandler;
        bool muted;
    };
public:
    EventImpl();

    ErrCode INTERFACE_FUNC addHandler(IEventHandler* eventHandler) override;
    ErrCode INTERFACE_FUNC removeHandler(IEventHandler* eventHandler) override;

    ErrCode INTERFACE_FUNC clear() override;
    ErrCode INTERFACE_FUNC getSubscriberCount(SizeT* count) override;

    ErrCode INTERFACE_FUNC trigger(IBaseObject* sender, IEventArgs* args) override;

    ErrCode INTERFACE_FUNC mute() override;
    ErrCode INTERFACE_FUNC unmute() override;

    ErrCode INTERFACE_FUNC muteListener(IEventHandler* eventHandler) override;
    ErrCode INTERFACE_FUNC unmuteListener(IEventHandler* eventHandler) override;

    // IFreezable
    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* isFrozen) const override;

    // IBaseObject
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;
private:
    using ConstIterator = std::vector<Handler>::const_iterator;
    using Iterator = std::vector<Handler>::iterator;

    ErrCode setMuted(IEventHandler* eventHandler, bool muted);

    std::atomic<bool> muted{};
    std::atomic<bool> frozen{};
    std::vector<Handler> handlers;

    mutable daq::mutex sync;
};

END_NAMESPACE_OPENDAQ
