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
#include <coretypes/delegate.hpp>
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coretypes/objectptr.h>
#include <coretypes/event_handler.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TSenderPtr, typename TEventArgsPtr>
class EventHandlerImpl : public ImplementationOf<IEventHandler>
{
public:
    using Subscription = delegate<void(TSenderPtr&, TEventArgsPtr&)>;

    explicit EventHandlerImpl(Subscription&& sub) : subscription(std::move(sub))
    {
    }

    ErrCode INTERFACE_FUNC handleEvent(IBaseObject* sender, IEventArgs* eventArgs) override
    {
        try
        {
            auto obj = sender != nullptr ? TSenderPtr::Borrow(sender) : TSenderPtr{};
            auto args = eventArgs != nullptr ? TEventArgsPtr::Borrow(eventArgs) : TEventArgsPtr{};

            subscription(obj, args);
        }
        catch (const DaqException& e)
        {
            return errorFromException(e);
        }
        catch (const std::exception&)
        {
            return OPENDAQ_ERR_GENERALERROR;
        }

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) override
    {
        if (hashCode == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Can not return by a null pointer.");
        }

        *hashCode = subscription.hashCode;
        return OPENDAQ_SUCCESS;
    }

private:
    Subscription subscription;
};

END_NAMESPACE_OPENDAQ
