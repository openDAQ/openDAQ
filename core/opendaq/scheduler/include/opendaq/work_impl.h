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
#include <opendaq/work.h>
#include <opendaq/work_ptr.h>
#include <opendaq/work_repetitive.h>
#include <opendaq/work_repetitive_internal.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>

#include <atomic>

BEGIN_NAMESPACE_OPENDAQ

template <class Callback>
class WorkImpl: public ImplementationOf<IWork>
{
public:
    explicit WorkImpl(const Callback& callback);
    explicit WorkImpl(Callback&& callback);

    ErrCode INTERFACE_FUNC execute() override;

private:
    Callback callback;
};

template <class Callback>
WorkImpl<Callback>::WorkImpl(const Callback& callback)
    : callback(callback)
{
}

template <class Callback>
WorkImpl<Callback>::WorkImpl(Callback&& callback)
    : callback(std::move(callback))
{
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkImpl<Callback>::execute()
{
    return daqTry([this] { callback(); });
}

template <class Callback>
class WorkRepetitiveImpl: public ImplementationOf<IWork, IWorkRepetitive, IWorkRepetitiveInternal>
{
public:
    explicit WorkRepetitiveImpl(const Callback& callback);
    explicit WorkRepetitiveImpl(Callback&& callback);
    explicit WorkRepetitiveImpl(SizeT intervalMs, const Callback& callback);
    explicit WorkRepetitiveImpl(SizeT intervalMs, Callback&& callback);

    ErrCode INTERFACE_FUNC executeRepetitively(Bool* repeatAfter) override;
    ErrCode INTERFACE_FUNC execute() override;
    ErrCode INTERFACE_FUNC getIntervalMs(SizeT* intervalMs) override;
    ErrCode INTERFACE_FUNC cancel(IWork* after) override;
    ErrCode INTERFACE_FUNC isCanceled(Bool* isCanceled) override;

    // Internal interface
    ErrCode INTERFACE_FUNC getOnStopCallback(IWork** onStopCallback) override;

private:
    SizeT intervalMs{0};
    std::atomic<bool> canceled{false};
    WorkPtr afterStoppedCallback;
    Callback callback;
};

template <class Callback>
WorkRepetitiveImpl<Callback>::WorkRepetitiveImpl(const Callback& callback)
    : callback(callback)
{
}

template <class Callback>
WorkRepetitiveImpl<Callback>::WorkRepetitiveImpl(Callback&& callback)
    : callback(std::move(callback))
{
}

template <class Callback>
WorkRepetitiveImpl<Callback>::WorkRepetitiveImpl(SizeT intervalMs, const Callback& callback)
    : intervalMs(intervalMs)
    , callback(callback)
{
}

template <class Callback>
WorkRepetitiveImpl<Callback>::WorkRepetitiveImpl(SizeT intervalMs, Callback&& callback)
    : intervalMs(intervalMs)
    , callback(std::move(callback))
{
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkRepetitiveImpl<Callback>::executeRepetitively(Bool* repeatAfter)
{
    OPENDAQ_PARAM_NOT_NULL(repeatAfter);
    *repeatAfter = False;

    using ReturnType = std::invoke_result_t<Callback>;
    if constexpr (std::is_same_v<ReturnType, bool> || std::is_same_v<ReturnType, Bool>)
    {
        return daqTry([this, repeatAfter] { *repeatAfter = callback(); });
    }
    else
    {
        return daqTry([this] { callback(); });
    }
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkRepetitiveImpl<Callback>::execute()
{
    Bool repeatAfter;
    return this->executeRepetitively(&repeatAfter);
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkRepetitiveImpl<Callback>::getIntervalMs(SizeT* intervalMsOut)
{
    OPENDAQ_PARAM_NOT_NULL(intervalMsOut);
    *intervalMsOut = intervalMs;
    return OPENDAQ_SUCCESS;
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkRepetitiveImpl<Callback>::cancel(IWork* after)
{
    if (after != nullptr)
        afterStoppedCallback = after;

    canceled.store(true);
    return OPENDAQ_SUCCESS;
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkRepetitiveImpl<Callback>::isCanceled(Bool* isCanceledOut)
{
    OPENDAQ_PARAM_NOT_NULL(isCanceledOut);
    *isCanceledOut = canceled.load();
    return OPENDAQ_SUCCESS;
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkRepetitiveImpl<Callback>::getOnStopCallback(IWork** onStopCallbackOut)
{
    OPENDAQ_PARAM_NOT_NULL(onStopCallbackOut);
    if (afterStoppedCallback.assigned())
        *onStopCallbackOut = afterStoppedCallback.detach();

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
