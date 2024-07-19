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
#include <coretypes/intfs.h>
#include <coreobjects/permissions.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/permissions_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class PermissionsImpl : public ImplementationOf<IPermissions>
{
public:
    explicit PermissionsImpl();
    explicit PermissionsImpl(Bool inherited, const DictPtr<IString, Int>& allowed, const DictPtr<IString, Int>& denied);

    ErrCode INTERFACE_FUNC getInherited(Bool* inherited) override;
    ErrCode INTERFACE_FUNC getAllowed(IDict** permissions) override;
    ErrCode INTERFACE_FUNC getDenied(IDict** permissions) override;

private:
    DictPtr<IString, Int> cloneDict(const DictPtr<IString, Int>& dict);

    Bool inherited;
    DictPtr<IString, Int> allowed;
    DictPtr<IString, Int> denied;
};

END_NAMESPACE_OPENDAQ
