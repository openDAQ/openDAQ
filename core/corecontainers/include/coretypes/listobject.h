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
#include <coretypes/baseobject.h>
#include <coretypes/iterator.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_containers
 * @defgroup types_list List
 * @{
 */

/*!
 * @brief Represents a heterogeneous collection of objects that can be individually accessed by index.
 */

DECLARE_OPENDAQ_INTERFACE(IList, IBaseObject)
{
    /*!
     * @brief Gets the element at a specific position.
     * @param index The zero-based index of the element to get. 
     * @param[out] obj The element at the specified index.
     *
     * The reference count of the element that is retrieved is incremented. The client is
     * responsible for calling `releaseRef` when the element is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC getItemAt(SizeT index, IBaseObject** obj) = 0;

    /*!
     * @brief Gets the number of elements contained in the list.
     * @param[out] size The number of elements contained in the list.
     */
    virtual ErrCode INTERFACE_FUNC getCount(SizeT* size) = 0;

    /*!
     * @brief Sets the element at a specific position.
     * @param index The zero-based index of the element to set.
     * @param obj The element to set at the specified index.
     *
     * The reference count of the element is incremented.
     */
    virtual ErrCode INTERFACE_FUNC setItemAt(SizeT index, IBaseObject* obj) = 0;

    /*!
     * @brief Inserts the element at the end of the list.
     * @param obj The element to insert.
     *
     * The reference count of the element is incremented.
     */
    virtual ErrCode INTERFACE_FUNC pushBack(IBaseObject* obj) = 0;

    /*!
     * @brief Inserts the element at the start of the list.
     * @param obj The element to insert.
     *
     * The reference count of the element is incremented.
     */
    virtual ErrCode INTERFACE_FUNC pushFront(IBaseObject* obj) = 0;

    /*!
     * @brief Inserts the element at the end of the list without incrementing the reference count.
     * @param obj The element to insert.
     *
     * The reference count of the element is not incremented. The client can use this method when it no
     * longer needs to access the element after calling the method.
     */
    virtual ErrCode INTERFACE_FUNC moveBack(IBaseObject * obj) = 0;

    /*!
     * @brief Inserts the element at the start of the list without incrementing the reference count.
     * @param obj The element to insert.
     *
     * The reference count of the element is not incremented. The client can use this method when it no
     * longer needs to access the element after calling the method.
     */
    virtual ErrCode INTERFACE_FUNC moveFront(IBaseObject* obj) = 0;

    /*!
     * @brief Gets the element from the end of the list.
     * @param[out] obj The extracted element.
     *
     * The reference count of the element that is retrieved is incremented. The client is
     * responsible for calling `releaseRef` when the element is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC popBack(IBaseObject** obj) = 0;

    /*!
     * @brief Gets the element from the start of the list.
     * @param[out] obj The extracted element.
     *
     * The reference count of the element that is retrieved is incremented. The client is
     * responsible for calling `releaseRef` when the element is no longer needed.
     */
    virtual ErrCode INTERFACE_FUNC popFront(IBaseObject** obj) = 0;

    /*!
     * @brief Inserts the element at a specific position.
     * @param index The zero-based index of the element to insert.
     * @param obj The element to insert at the specified index.
     *
     * The reference count of the element is incremented.
     */
    virtual ErrCode INTERFACE_FUNC insertAt(SizeT index, IBaseObject* obj) = 0;

    /*!
     * @brief Removes the element at a specific position.
     * @param index The zero-based index of the element to remove.
     * @param[out] obj The removed element.
     *
     * The client is responsible for calling `releaseRef` when the element is no longer needed.
     * If the client does not need the element after it is removed, it should call `delete` method.
     */
    virtual ErrCode INTERFACE_FUNC removeAt(SizeT index, IBaseObject** obj) = 0;

    /*!
     * @brief Deletes the element at a specific position.
     * @param index The zero-based index of the element to remove.
     *
     * If the client needs the element deleted, it should use `removeAt` method.
     */
    virtual ErrCode INTERFACE_FUNC deleteAt(SizeT index) = 0;

    /*!
     * @brief Removes all elements from the list.
     */
    virtual ErrCode INTERFACE_FUNC clear() = 0;

    /*!
     * @brief Creates and returns the start iterator of the list.
     * @param[out] iterator The start iterator.
     *
     * Use iterators to iterate through the elements.
     */
    virtual ErrCode INTERFACE_FUNC createStartIterator(IIterator** iterator) = 0;

    /*!
     * @brief Creates and returns the stop iterator of the list.
     * @param[out] iterator The stop iterator.
     *
     * Use iterators to iterate through the elements.
     */
    virtual ErrCode INTERFACE_FUNC createEndIterator(IIterator** iterator) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, List)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ListWithElementType, IList,
    IntfID, id
)

END_NAMESPACE_OPENDAQ
