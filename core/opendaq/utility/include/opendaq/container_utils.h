/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <coretypes/objectptr.h>
#include <coretypes/cloneable.h>
#include <coretypes/ctutils.h>
#include <coretypes/dictptr.h>
#include <coretypes/exceptions.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Gets a frozen copy of a container, such as a dictionary or a list.
 * @param container The container to copy; an unassigned one gives an unassigned copy.
 *
 * The copy keeps the order of the items. A cloneable item is cloned, any other item is shared.
 */
template <typename TPtr>
TPtr FrozenClone(const TPtr& container)
{
    if (!container.assigned())
        return TPtr();

    BaseObjectPtr cloned;
    checkErrorInfo(container.template asPtr<ICloneable>()->clone(&cloned));

    TPtr copy = cloned.template asPtr<typename TPtr::DeclaredInterface>();
    copy.freeze();
    return copy;
}

/*!
 * @brief Gets the first value of a dictionary in iteration order.
 * @param dict The dictionary; may be unassigned.
 * @returns The value, or unassigned if the dictionary is empty or unassigned.
 */
template <typename T, typename KeyT, typename ValueT, typename KeyPtr, typename ValuePtr>
ValuePtr FirstValue(const DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>& dict)
{
    if (dict.assigned())
    {
        for (const auto& [key, value] : dict)
            return value;
    }

    return ValuePtr();
}

/*!
 * @brief Checks that a dictionary holds objects of one interface, each keyed by its own id.
 * @tparam TInterface The interface of the values, which has getId (e.g. ITransportOption).
 * @param dict The dictionary to check.
 * @throws InvalidTypeException if a key is not a string or a value does not implement the interface.
 * @throws InvalidValueException if a key is not the id of its value.
 */
template <typename TInterface, typename TPtr = typename InterfaceToSmartPtr<TInterface>::SmartPtr>
void CheckKeyedByIds(IDict* dict)
{
    for (const auto& [key, item] : DictPtr<IBaseObject, IBaseObject>::Borrow(dict))
    {
        const auto id = key.asPtrOrNull<IString, StringPtr>();
        const auto value = item.template asPtrOrNull<TInterface, TPtr>();
        if (!id.assigned() || !value.assigned())
            DAQ_THROW_EXCEPTION(InvalidTypeException,
                                "The dictionary holds an entry that is not an object of the expected type keyed by a string");

        if (id != value.getId())
            DAQ_THROW_EXCEPTION(InvalidValueException, R"(The object "{}" is keyed by "{}")", value.getId(), id);
    }
}

END_NAMESPACE_OPENDAQ
