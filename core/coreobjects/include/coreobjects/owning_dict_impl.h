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
#include <coretypes/common.h>
#include <coretypes/dictobject_impl.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class OwningDictImpl : public DictImpl
{
public:
    OwningDictImpl(PropertyObjectPtr owner, StringPtr ref);

    ErrCode INTERFACE_FUNC set(IBaseObject* key, IBaseObject* value) override;

    ErrCode INTERFACE_FUNC remove(IBaseObject* key, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC deleteItem(IBaseObject* key) override;

    ErrCode INTERFACE_FUNC clear() override;

private:
    StringPtr ref;
    PropertyObjectPtr owner;

    ErrCode removeOwner(IBaseObject* value) const;
};

END_NAMESPACE_OPENDAQ
