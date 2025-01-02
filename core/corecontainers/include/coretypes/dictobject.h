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
#include <coretypes/baseobject.h>
#include <coretypes/listobject.h>
#include <coretypes/iterable.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_containers
 * @defgroup types_dict Dict
 * @{
 */

/*!
 * @brief Represents a collection of key/value pairs.
 */

DECLARE_OPENDAQ_INTERFACE(IDict, IBaseObject)
{
    /*!
     * @brief Gets the element with the specified key.
     * @param key The key of the element to get.
     * @param[out] value The element with the specified key.
     *
     * The reference count of the element that is retrieved is incremented. The client is
     * responsible for calling `releaseRef` when the element is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC get(IBaseObject* key, IBaseObject** value) = 0;

    /*!
     * @brief Sets the element with the specified key.
     * @param key The key of the element to set.
     * @param value The element with the specified key.
     *
     * The reference count of the key and the element is incremented.
     */
    virtual ErrCode INTERFACE_FUNC set(IBaseObject* key, IBaseObject* value) = 0;

    /*!
     * @brief Removes the element with the specified key.
     * @param key The key of the element to remove.
     * @param[out] value The element with the specified key.
     *
     * The client is responsible for calling `releaseRef` when the element is no longer needed.
     * If the client does not need the element after it is removed, it should call `delete` method.
     */
    virtual ErrCode INTERFACE_FUNC remove(IBaseObject* key, IBaseObject** value) = 0;

    /*!
     * @brief Deletes the element with the specified key.
     * @param key The key of the element to delete.
     *
     * If the client needs the element deleted, it should use `removeAt` method.
     */
    virtual ErrCode INTERFACE_FUNC deleteItem(IBaseObject* key) = 0;

    /*!
     * @brief Removes all elements from the list.
     */
    virtual ErrCode INTERFACE_FUNC clear() = 0;

    /*!
     * @brief Gets the number of elements contained in the dictionary.
     * @param[out] size The number of elements contained in the dictionary.
     */
    virtual ErrCode INTERFACE_FUNC getCount(SizeT * size) = 0;

    /*!
     * @brief Checks if the element with the specified key exists in the dictionary.
     * @param key The key of the element to check.
     * @param[out] hasKey True if the element exists, False otherwise.
     */
    virtual ErrCode INTERFACE_FUNC hasKey(IBaseObject* key, Bool* hasKey) = 0;

    /*!
     * @brief Gets the list of all keys in the dictionary.
     * @param[out] keys The list of the keys.
     *
     * The order of the keys is not defined.
     *
     * The client is responsible for calling `releaseRef` when the list is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC getKeyList(IList** keys) = 0;

    /*!
     * @brief Gets the list of all elements in the dictionary.
     * @param[out] values The list of the elements.
     *
     * The order of the elements is not defined.
     *
     * The client is responsible for calling `releaseRef` when the list is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC getValueList(IList** values) = 0;

    /*!
     * @brief Gets the iterable interface of the keys.
     * @param[out] iterable The iterable interface of the keys.
     *
     * The Iterable interface enables iteration through the keys.
     *
     * The client is responsible for calling `releaseRef` when the interface is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC getKeys(IIterable** iterable) = 0;

    /*!
     * @brief Gets the iterable interface of the elements.
     * @param[out] iterable The iterable interface of the elements.
     *
     * The Iterable interface enables iteration through the elements.
     *
     * The client is responsible for calling `releaseRef` when the interface is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC getValues(IIterable** iterable) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Dict)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DictWithExpectedTypes, IDict,
    IntfID, keyType,
    IntfID, valueType
)

END_NAMESPACE_OPENDAQ
