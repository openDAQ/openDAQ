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
#include <coretypes/common.h>
#include <coreobjects/ownable_ptr.h>
#include <coretypes/listobject_impl.h>
#include <coretypes/weakrefptr.h>

BEGIN_NAMESPACE_OPENDAQ

class OwningListImpl final : public ListImpl
{
public:
    OwningListImpl(IPropertyObject* owner, StringPtr ref);

    ErrCode INTERFACE_FUNC setItemAt(SizeT index, IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC insertAt(SizeT index, IBaseObject* obj) override;

    ErrCode INTERFACE_FUNC pushBack(IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC pushFront(IBaseObject* obj) override;

    ErrCode INTERFACE_FUNC moveBack(IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC moveFront(IBaseObject* obj) override;

    ErrCode INTERFACE_FUNC popBack(IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC popFront(IBaseObject** obj) override;

    ErrCode INTERFACE_FUNC removeAt(SizeT index, IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC deleteAt(SizeT index) override;
    ErrCode INTERFACE_FUNC clear() override;
private:
    StringPtr ref;
    WeakRefPtr<IPropertyObject> owner;

    ErrCode removeOwner(IBaseObject* value) const;
    ErrCode setOwner(IBaseObject* value) const;
};

END_NAMESPACE_OPENDAQ
