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
#include <opendaq/deleter.h>
#include <coretypes/intfs.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Callback>
class DeleterImpl : public ImplementationOf<IDeleter>
{
public:
    explicit DeleterImpl(const Callback& callback);
    explicit DeleterImpl(Callback&& callback);

    ErrCode INTERFACE_FUNC deleteMemory(void* address) override;

private:
    Callback callback;
};

template <class Callback>
DeleterImpl<Callback>::DeleterImpl(const Callback& callback)
    : callback(callback)
{
}

template <class Callback>
DeleterImpl<Callback>::DeleterImpl(Callback&& callback)
    : callback(std::move(callback))
{
}

template <class Callback>
ErrCode DeleterImpl<Callback>::deleteMemory(void* address)
{
    callback(address);
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
