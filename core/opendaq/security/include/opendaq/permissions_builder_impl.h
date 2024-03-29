/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/permissions_builder.h>
#include <opendaq/permissions_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class PermissionsBuilderImpl : public ImplementationOf<IPermissionsBuilder>
{
public:
    explicit PermissionsBuilderImpl();

    ErrCode INTERFACE_FUNC inherit(Bool inherit) override;
    ErrCode INTERFACE_FUNC set(IString* groupId, Int permissionFlags) override;
    ErrCode INTERFACE_FUNC allow(IString* groupId, Int permissionFlags) override;
    ErrCode INTERFACE_FUNC deny(IString* groupId, Int permissionFlags) override;
    ErrCode INTERFACE_FUNC extend(IPermissions* config) override;
    ErrCode INTERFACE_FUNC build(IPermissions** configOut) override;

private:
    Bool inherited;
    DictPtr<IString, Int> allowed;
    DictPtr<IString, Int> denied;
};

END_NAMESPACE_OPENDAQ
