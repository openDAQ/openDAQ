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
#include <coretypes/dict_element_type.h>
#include <coretypes/iterator_base_impl.h>

BEGIN_NAMESPACE_OPENDAQ

struct KeyValueSelector;

template <typename T>
class DictIterator : public IteratorBaseImpl<T, IDictElementType, KeyValueSelector>
{
public:
    using Super = IteratorBaseImpl<T, IDictElementType, KeyValueSelector>;
    using IteratorType = typename Super::IteratorType;

    DictIterator(IBaseObject* coreContainer, IteratorType it, IteratorType end, const IntfID& idKey, const IntfID& idValue)
        : Super(coreContainer, it, end)
        , keyId(idKey)
        , valueId(idValue)
    {
    }

    ErrCode INTERFACE_FUNC getKeyInterfaceId(IntfID* idKey) override
    {
        if (idKey == nullptr)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Key-id output parameter must not be null.");

        *idKey = keyId;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getValueInterfaceId(IntfID* idValue) override
    {
        if (idValue == nullptr)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Value-id output parameter must not be null.");

        *idValue = valueId;
        return OPENDAQ_SUCCESS;
    }

private:
    const IntfID& keyId;
    const IntfID& valueId;
};

template <typename T, typename Selector>
class DictListIterator : public IteratorBaseImpl<T, IListElementType, Selector>
{
public:
    using Super = IteratorBaseImpl<T, IListElementType, Selector>;
    using IteratorType = typename Super::IteratorType;

    DictListIterator(IBaseObject* coreContainer, IteratorType it, IteratorType end, const IntfID& id)
        : Super(coreContainer, it, end)
        , id(id)
    {
    }

    ErrCode INTERFACE_FUNC getElementInterfaceId(IntfID* elementId) override
    {
        if (elementId == nullptr)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Element-Id output parameter must not be null.");

        *elementId = id;
        return OPENDAQ_SUCCESS;
    }

private:
    const IntfID& id;
};

END_NAMESPACE_OPENDAQ
