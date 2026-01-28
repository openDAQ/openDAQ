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

#include <opendaq/sync_interface_base_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class ClockSyncInterfaceImpl : public SyncInterfaceBaseImpl<>
{
public:
    using Super = SyncInterfaceBaseImpl<>;

    explicit ClockSyncInterfaceImpl();

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool isSource) override;
};

inline ClockSyncInterfaceImpl::ClockSyncInterfaceImpl()
    : Super("ClockSyncInterface")
{
}

inline ErrCode ClockSyncInterfaceImpl::setAsSource(Bool isSource)
{
    auto lock = getRecursiveConfigLock2();
    if (isSource)
    {
        setModeOptions(Dict<IInteger, IString>({{static_cast<Int>(SyncMode::Input), "Input"}}));
        this->objPtr.setPropertyValue("Mode", static_cast<Int>(SyncMode::Input));
    }
    else
    {
        setModeOptions(Dict<IInteger, IString>({{static_cast<Int>(SyncMode::Off), "Off"}}));
        this->objPtr.setPropertyValue("Mode", static_cast<Int>(SyncMode::Off));
    }
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
