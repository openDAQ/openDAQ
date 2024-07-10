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
#include <cstdint>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/iterator.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_containers
 * @defgroup types_iterable Iterable
 * @{
 */

/*!
 * @brief An iterable object can construct iterators and use them to iterate through items.
 *
 * Use this interface to get the start and end iterators. Use iterators to iterate through
 * available items. Containers such as lists and dictionaries usually implement this interface.
 */
DECLARE_OPENDAQ_INTERFACE(IIterable, IBaseObject)
{
    /*!
     * @brief Creates and returns the object's start iterator.
     * @param[out] iterator The object's start iterator.
     */
    virtual ErrCode INTERFACE_FUNC createStartIterator(IIterator** iterator) = 0;

    /*!
     * @brief Creates and returns the object's end iterator.
     * @param[out] iterator The object's end iterator.
     */
    virtual ErrCode INTERFACE_FUNC createEndIterator(IIterator** iterator) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
