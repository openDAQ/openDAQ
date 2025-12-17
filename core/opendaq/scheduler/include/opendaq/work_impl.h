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
#include <opendaq/work.h>
#include <opendaq/work_repetitive.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>

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
class WorkRepetitiveImpl: public ImplementationOf<IWork, IWorkRepetitive>
{
public:
    explicit WorkRepetitiveImpl(const Callback& callback);
    explicit WorkRepetitiveImpl(Callback&& callback);

    ErrCode INTERFACE_FUNC executeRepetitively(Bool* repeatAfter) override;
    ErrCode INTERFACE_FUNC execute() override;

private:
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

END_NAMESPACE_OPENDAQ
