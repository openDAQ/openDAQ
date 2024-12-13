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
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coretypes/objectptr.h>
#include <coretypes/event_handler.h>


using EventHandlerSignature = void(daq::IBaseObject*, daq::IEventArgs*);
using EventHandler = std::function<EventHandlerSignature>;

BEGIN_NAMESPACE_OPENDAQ

class PyEventHandlerImpl : public ImplementationOf<IEventHandler>
{
public:
    using Subscription = EventHandler;

    explicit PyEventHandlerImpl(Subscription&& sub) : subscription(std::move(sub))
    {
    }

    ErrCode INTERFACE_FUNC handleEvent(IBaseObject* sender, IEventArgs* eventArgs) override
    {
        try
        {
            if(sender) sender->addRef();
            if(eventArgs) eventArgs->addRef();

            subscription(sender, eventArgs);
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

private:
    Subscription subscription;
};

END_NAMESPACE_OPENDAQ
