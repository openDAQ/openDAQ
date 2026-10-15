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
#include <coretypes/coretypes.h>
#include <coretypes/cloneable.h>
#include <coretypes/event_handler_ptr.h>
#include <coretypes/utility_sync.h>
#include <atomic>
#include <condition_variable>
#include <memory>
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

class EventImpl : public ImplementationOf<IEvent, IFreezable, ICloneable>
{
    struct Handler
    {
        Handler(EventHandler eventHandler, SizeT hashCode);

        const EventHandler eventHandler;
        const SizeT hashCode;
        std::atomic<bool> removed{false}; // Lets an event trigger that is still walking an older list skip a handler that was removed after.
        std::atomic<SizeT> inFlight{0};   // Lets removeHandler() and clear() return only once the handler has stopped running on other threads.
    };

    // Muted state is taken at the trigger time instead of just before handler invocation
    struct HandlerEntry
    {
        std::shared_ptr<Handler> handler;
        bool muted;
    };
    using HandlerList = std::vector<HandlerEntry>;

    class CallInProgress;
public:
    EventImpl();

    ErrCode INTERFACE_FUNC addHandler(IEventHandler* eventHandler) override;
    ErrCode INTERFACE_FUNC removeHandler(IEventHandler* eventHandler) override;

    ErrCode INTERFACE_FUNC clear() override;
    ErrCode INTERFACE_FUNC getSubscriberCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC getSubscribers(IList** subscribers) override;

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

    // IClonable
    ErrCode INTERFACE_FUNC clone(IBaseObject** cloned) override;
private:
    using ConstIterator = std::vector<Handler>::const_iterator;
    using Iterator = std::vector<Handler>::iterator;

    ErrCode setMuted(IEventHandler* eventHandler, bool muted);
    std::shared_ptr<const HandlerList> getHandlers() const;
    void finishCall(Handler& handler);
    void waitForCallsOnOtherThreads(std::unique_lock<daq::mutex>& lock, const std::vector<std::shared_ptr<Handler>>& removedHandlers);

    std::atomic<bool> muted{};
    std::atomic<bool> frozen{};

    // Replaced as a whole when handlers are added or removed, so trigger() iterates a list without holding the lock.
    // Handlers are called with the lock released so they can use any event and take other locks without making this lock part of a cycle.
    std::shared_ptr<const HandlerList> handlers;

    // Lets a finishing call take the lock to notify only when removeHandler() or clear() is waiting.
    std::atomic<SizeT> waitingForCalls{0};

    mutable daq::mutex sync;
    std::condition_variable_any callFinished;
};

END_NAMESPACE_OPENDAQ
