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
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coreobjects/end_update_event_args.h>
#include <coreobjects/property_ptr.h>
#include <coretypes/event_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class EndUpdateEventArgsImpl : public EventArgsBase<IEndUpdateEventArgs>
{
public:
    explicit EndUpdateEventArgsImpl(const ListPtr<IString>& properties, Bool isParentUpdating);

    ErrCode INTERFACE_FUNC getProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC getIsParentUpdating(Bool* isParentUpdating) override;
private:
    ListPtr<IString> properties;
    Bool isParentUpdating;
};

END_NAMESPACE_OPENDAQ
