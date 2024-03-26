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
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coretypes/event_args_impl.h>
#include <opendaq/subscription_event_args.h>

BEGIN_NAMESPACE_OPENDAQ

class SubscriptionEventArgsImpl : public EventArgsBase<ISubscriptionEventArgs>
{
public:
    explicit SubscriptionEventArgsImpl(const StringPtr& streamingConnectionString,
                                       SubscriptionEventType eventType);

    ErrCode INTERFACE_FUNC getStreamingConnectionString(IString** streamingConnectionString) override;
    ErrCode INTERFACE_FUNC getSubscriptionEventType(SubscriptionEventType* eventType) override;

private:
    StringPtr streamingConnectionString;
    SubscriptionEventType eventType;
};

END_NAMESPACE_OPENDAQ
