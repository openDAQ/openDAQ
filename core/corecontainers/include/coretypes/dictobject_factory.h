/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/dictobject.h>
#include <coretypes/dict_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <class KeyT, class ValueT>
inline DictObjectPtr<IDict, KeyT, ValueT> Dict()
{
    using KeyPtr = typename InterfaceOrTypeToSmartPtr<KeyT>::SmartPtr;
    using ValuePtr = typename InterfaceOrTypeToSmartPtr<ValueT>::SmartPtr;
    using DictionaryPtr = DictObjectPtr<IDict, KeyT, ValueT, KeyPtr, ValuePtr>;

    DictionaryPtr obj(DictWithExpectedTypes_Create(KeyPtr::DeclaredInterface::Id, ValuePtr::DeclaredInterface::Id));
    return obj;
}

template <typename KeyT, typename ValueT>
static DictObjectPtr<IDict, KeyT, ValueT> Dict(std::initializer_list<std::pair<const typename InterfaceToSmartPtr<KeyT>::SmartPtr,
                                                                                     typename InterfaceToSmartPtr<ValueT>::SmartPtr>> init)
{
    using KeyPtr = typename InterfaceOrTypeToSmartPtr<KeyT>::SmartPtr;
    using ValuePtr = typename InterfaceOrTypeToSmartPtr<ValueT>::SmartPtr;
    using DictionaryPtr = DictObjectPtr<IDict, KeyT, ValueT, KeyPtr, ValuePtr>;

    DictionaryPtr obj(DictWithExpectedTypes_Create(KeyPtr::DeclaredInterface::Id, ValuePtr::DeclaredInterface::Id));
    for (auto& [k, v] : init)
    {
        obj.set(k, v);
    }

    return obj;
}

template <class KeyT, class ValueT>
using DictPtr = DictObjectPtr<IDict, KeyT, ValueT>;

END_NAMESPACE_OPENDAQ
