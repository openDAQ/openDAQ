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
#include <opendaq/awaitable.h>
#include <coretypes/intfs.h>
#include <opendaq/task_flow.h>
#include <coretypes/baseobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TReturn = void>
class AwaitableImpl : public ImplementationOf<IAwaitable>
{
public:
    using Future = tf::Future<TReturn>;

    explicit AwaitableImpl(Future future);

    ErrCode INTERFACE_FUNC cancel(Bool* canceled) override;
    ErrCode INTERFACE_FUNC wait() override;
    ErrCode INTERFACE_FUNC getResult(IBaseObject** result) override;
    ErrCode INTERFACE_FUNC hasCompleted(Bool* finished) override;
private:
    Future future;
    std::atomic<bool> completed;
};

using Awaitable = AwaitableImpl<>;
using AwaitableFunc = AwaitableImpl<std::optional<BaseObjectPtr>>;

END_NAMESPACE_OPENDAQ
