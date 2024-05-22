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
#include <opendaq/work.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Callback>
class WorkImpl: public ImplementationOf<IWork>
{
public:
    using Super = ImplementationOf<IWork>;

    explicit WorkImpl(const Callback& callback);
    explicit WorkImpl(Callback&& callback);

    ErrCode INTERFACE_FUNC execute() override;

    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

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
ErrCode INTERFACE_FUNC WorkImpl<Callback>::queryInterface(const IntfID& id, void** intf)
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IWork::Id)
    {
        *intf = static_cast<IWork*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    return Super::queryInterface(id, intf);
}

template <class Callback>
ErrCode INTERFACE_FUNC WorkImpl<Callback>::borrowInterface(const IntfID& id, void** intf) const
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IWork::Id)
    {
        *intf = const_cast<IWork*>(static_cast<const IWork*>(this));

        return OPENDAQ_SUCCESS;
    }

    return Super::borrowInterface(id, intf);
}

END_NAMESPACE_OPENDAQ
