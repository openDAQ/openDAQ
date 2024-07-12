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
#include <coretypes/iterable.h>
#include <coretypes/objectptr.h>
#include <coretypes/iterator_support.h>

BEGIN_NAMESPACE_OPENDAQ

template <class TValueInterface, class TValuePtr = typename InterfaceToSmartPtr<TValueInterface>::SmartPtr>
class IterablePtr;

template <>
struct InterfaceToSmartPtr<IIterable>
{
    typedef IterablePtr<IBaseObject> SmartPtr;
};

/*!
 * @addtogroup types_iterable
 * @{
 */

/*!
 * @brief An iterable object can construct iterators and use them to iterate through items.
 *
 * Use this object to get the start and end iterators. Use iterators to iterate through
 * available items. Containers such as lists and dictionaries usually implement this interface.
 */
template <class TValueInterface, class TValuePtr>
class IterablePtr : public ObjectPtr<IIterable>
{
public:
    using ObjectPtr<IIterable>::ObjectPtr;

    static IterablePtr<TValueInterface, TValuePtr> Adopt(IIterable* obj)
    {
        IterablePtr<TValueInterface, TValuePtr> objPtr = std::move(obj);
        return objPtr;
    }

    /*!
     * @brief Creates and returns the object's start iterator.
     * @return The object's start iterator.
     */
    NativeIterator<TValuePtr> createStartIteratorInterface() const;

    /*!
     * @brief Creates and returns the object's end iterator.
     * @return The object's stop iterator.
     */
    NativeIterator<TValuePtr> createEndIteratorInterface() const;

    /*!
     * @brief Creates and returns the object's start iterator.
     * @return The object's start iterator.
     */
    NativeIterator<TValuePtr> begin() const;

    /*!
     * @brief Creates and returns the object's end iterator.
     * @return The object's stop iterator.
     */
    NativeIterator<TValuePtr> end() const;

protected:
};

/*!@}*/

template <class U, class TValuePtr>
NativeIterator<TValuePtr> IterablePtr<U, TValuePtr>::createStartIteratorInterface() const
{
    IIterator* iterator = ObjectPtr<IIterable>::createStartIteratorInterface();
    return NativeIterator<TValuePtr>(std::move(iterator));
}

template <class U, class TValuePtr>
NativeIterator<TValuePtr> IterablePtr<U, TValuePtr>::createEndIteratorInterface() const
{
    IIterator* iterator = ObjectPtr<IIterable>::createEndIteratorInterface();
    return NativeIterator<TValuePtr>(std::move(iterator));
}

template <class U, class TValuePtr>
NativeIterator<TValuePtr> IterablePtr<U, TValuePtr>::begin() const
{
    return createStartIteratorInterface();
}

template <class U, class TValuePtr>
NativeIterator<TValuePtr> IterablePtr<U, TValuePtr>::end() const
{
    return createEndIteratorInterface();
}

END_NAMESPACE_OPENDAQ
