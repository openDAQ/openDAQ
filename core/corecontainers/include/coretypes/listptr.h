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
#include <coretypes/objectptr.h>
#include <coretypes/iterator_support.h>
#include <coretypes/list_element_type.h>

#include <vector>

BEGIN_NAMESPACE_OPENDAQ

template <class T, class TValueInterface, class TValuePtr = typename InterfaceToSmartPtr<TValueInterface>::SmartPtr>
class ListObjectPtr;

template <>
struct InterfaceToSmartPtr<IList>
{
    typedef ListObjectPtr<IList, IBaseObject> SmartPtr;
};

/*!
 * @addtogroup types_list
 * @{
 */

/*!
 * @brief Represents a heterogeneous collection of objects that can be individually accessed by index.
 */
template <class T, class TValueInterface, class TValuePtr>
class ListObjectPtr : public ObjectPtr<T>
{
public:
    using ObjectPtr<T>::ObjectPtr;

    using IteratorType = NativeIterator<TValuePtr>;

    // To satisfy Container trait
    using iterator_category = typename IteratorType::iterator_category;
    using value_type = typename IteratorType::value_type;
    using reference = typename IteratorType::reference;
    using const_reference = typename IteratorType::const_reference;
    using iterator = IteratorType;
    // using const_iterator = const IteratorType;
    using difference_type = typename IteratorType::difference_type;
    using size_type = std::size_t;

    ListObjectPtr()
        : ObjectPtr<IList>()
    {
    }

    ListObjectPtr(const ObjectPtr<IList>& ptr)
        : ObjectPtr<IList>(ptr)
    {
    }

    ListObjectPtr(ObjectPtr<IList>&& ptr)
        : ObjectPtr<IList>(ptr)
    {
    }

    template <typename V, std::enable_if_t<std::is_convertible_v<V, TValuePtr>, int> = 0>
    ListObjectPtr(const std::vector<V>& vector)
    {
        ErrCode err = createListWithElementType(&this->object, TValuePtr::DeclaredInterface::Id);
        checkErrorInfo(err);

        for (auto& element : vector)
        {
            this->pushBack(element);
        }
    }

    template <
        typename IteratorBeg,
        typename IteratorEnd,
        typename = std::enable_if_t <
            std::is_convertible_v<typename std::iterator_traits<IteratorBeg>::iterator_category, std::input_iterator_tag> &&
            std::is_convertible_v<typename std::iterator_traits<IteratorEnd>::iterator_category, std::input_iterator_tag>
        >
    >
    ListObjectPtr(IteratorBeg first, IteratorEnd last)
    {
        ErrCode err = createListWithElementType(&this->object, TValuePtr::DeclaredInterface::Id);
        checkErrorInfo(err);

        for (; first != last; ++first)
        {
            this->pushBack(*first);
        }
    }

    template <typename V, std::enable_if_t<std::is_convertible_v<V, TValuePtr>, int> = 0>
    ListObjectPtr(const std::initializer_list<V>& list)
    {
        ErrCode err = createListWithElementType(&this->object, TValuePtr::DeclaredInterface::Id);
        checkErrorInfo(err);

        for (auto& element : list)
        {
            this->pushBack(element);
        }
    }

    template <typename V, std::enable_if_t<std::is_convertible_v<V, TValuePtr>, int> = 0>
    static ListObjectPtr FromVector(const std::vector<V>& vector)
    {
        ListObjectPtr ptr;
        ErrCode err = createListWithElementType(&ptr, TValuePtr::DeclaredInterface::Id);
        checkErrorInfo(err);

        for (auto& element : vector)
        {
            ptr.pushBack(element);
        }

        return ptr;
    }

    static ListObjectPtr<T, TValueInterface, TValuePtr> Borrow(T*&& obj) = delete;

    static ListObjectPtr<T, TValueInterface, TValuePtr> Borrow(T*& obj)
    {
        ListObjectPtr<T, TValueInterface, TValuePtr> objPtr;
        objPtr.object = obj;
        objPtr.borrowed = true;
        return objPtr;
    }

    static ListObjectPtr<T, TValueInterface, TValuePtr> Adopt(T* obj)
    {
        ListObjectPtr<T, TValueInterface, TValuePtr> objPtr;
        objPtr.object = obj;
        objPtr.borrowed = false;
        return objPtr;
    }

    template <class U, class V = TValueInterface, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int> = 0>
    static ListObjectPtr<T, V, typename InterfaceToSmartPtr<V>::SmartPtr> Borrow(U*&& obj) = delete;

    template <class U, class V = TValueInterface, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int> = 0>
    static ListObjectPtr<T, V, typename InterfaceToSmartPtr<V>::SmartPtr> Borrow(U*& obj)
    {
        ListObjectPtr<T, V, typename InterfaceToSmartPtr<V>::SmartPtr> objPtr;

        T* intf;
        auto res = obj->borrowInterface(T::Id, reinterpret_cast<void**>(&intf));
        checkErrorInfo(res);

        objPtr.object = intf;
        objPtr.borrowed = true;
        return objPtr;
    }

   /*!
     * @brief Gets the element at a specific position.
     * @param index The zero-based index of the element to get.
     * @return The element at the specified index.
     */
    TValuePtr getItemAt(size_t index) const;

    /*!
     * @brief Gets the number of elements contained in the list.
     * @return The number of elements contained in the list.
     */
    SizeT getCount() const;

    /*!
     * @brief Returns true if there are no elements in the list.
     * @return True if no elements are in the list, False otherwise.
     */
    SizeT empty() const;

    template <typename TEnum, std::enable_if_t<std::is_enum_v<TEnum>, int> = 0>
    void setItemAt(size_t index, const TEnum& obj);

    /*!
     * @brief Sets the element at a specific position.
     * @param index The zero-based index of the element to set.
     * @param obj The element to set at the specified index.
     */
    void setItemAt(size_t index, const TValuePtr& obj);

    /*!
     * @brief Inserts the element at the end of the list.
     * @param obj The element to insert.
     */
    void pushBack(const TValuePtr& obj);

    /*!
     * @brief Inserts the element at the end of the list.
     * @param obj The element to insert.
     */
    void pushBack(TValuePtr&& obj);

    /*!
     * @brief Inserts the element at the start of the list.
     * @param obj The element to insert.
     */
    void pushFront(const TValuePtr& obj);

    /*!
     * @brief Inserts the element at the start of the list.
     * @param obj The element to insert.
     */
    void pushFront(TValuePtr&& obj);


    /*!
     * @brief Gets the element from the end of the list.
     * @return The extracted element.
     */

    TValuePtr popBack();
    /*!
     * @brief Gets the element from the start of the list.
     * @return The extracted element.
     */
    TValuePtr popFront();

    /*!
     * @brief Inserts the element at a specific position.
     * @param index The zero-based index of the element to insert.
     * @param obj The element to insert at the specified index.
     */
    void insertAt(size_t index, const TValuePtr& obj);

    /*!
     * @brief Removes the element at a specific position.
     * @param index The zero-based index of the element to remove.
     * @return The removed element.
     *
     * If the client does not need the element after it is removed, it should call `delete` method.
     */
    TValuePtr removeAt(size_t index);

    /*!
     * @brief Deletes the element at a specific position.
     * @param index The zero-based index of the element to remove.
     *
     * If the client needs the element deleted, it should use `removeAt` method.
     */
    void deleteAt(size_t index);

    /*!
     * @brief Removes all elements from the list.
     */
    void clear();

    void unsafePushBack(const TValuePtr& obj);
    void unsafePushBack(TValuePtr&& obj);

    TValuePtr operator[](size_t index);
    TValuePtr operator[](size_t index) const;
    TValuePtr operator[](int index);
    TValuePtr operator[](int index) const;

    /*!
     * @brief Creates start iterator for the list.
     */
    IteratorType begin() const;

    /*!
     * @brief Creates end iterator for the list.
     */
    IteratorType end() const;

    /*!
     * @brief Returns the interface id of the expected list element type.
     * @returns The interface id of the expected element type otherwise returns the id of `IUnknown`.
     */
    [[nodiscard]] IntfID getElementInterfaceId() const;

    /*!
     * @brief Returns a copy of list as std::vector.
     */
    std::vector<TValuePtr> toVector()
    {
        std::vector<TValuePtr> vector;

        SizeT size;
        IBaseObject* obj;
        this->object->getCount(&size);

        for (SizeT i = 0; i < size; i++)
        {
            this->object->getItemAt(i, &obj);
            vector.push_back(obj);
        }

        return vector;
    }
};

/*!@}*/

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::getItemAt(size_t index) const
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    IBaseObject* obj;
    ErrCode errCode = this->object->getItemAt(index, &obj);
    checkErrorInfo(errCode);

    return V(std::move(obj));
}

template <class T, class U, class V>
SizeT ListObjectPtr<T, U, V>::getCount() const
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    SizeT size;
    checkErrorInfo(this->object->getCount(&size));
    return size;
}

template <class T, class TValueInterface, class TValuePtr>
SizeT ListObjectPtr<T, TValueInterface, TValuePtr>::empty() const
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    SizeT size;
    checkErrorInfo(this->object->getCount(&size));
    return size == 0;
}

template <class T, class TValueInterface, class TValuePtr>
template <typename TEnum, std::enable_if_t<std::is_enum_v<TEnum>, int>>
void ListObjectPtr<T, TValueInterface, TValuePtr>::setItemAt(size_t index, const TEnum& obj)
{
    return setItemAt(index, Int(obj));
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::setItemAt(size_t index, const V& obj)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    ErrCode errCode = this->object->setItemAt(index, obj);
    checkErrorInfo(errCode);
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::pushBack(const V& obj)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    checkErrorInfo(this->object->pushBack(obj));
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::pushBack(V&& obj)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    checkErrorInfo(this->object->moveBack(obj));
    obj.detach();
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::unsafePushBack(const V& obj)
{
    this->object->pushBack(obj);
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::unsafePushBack(V&& obj)
{
    this->object->moveBack(obj);
    obj.detach();
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::pushFront(const V& obj)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    checkErrorInfo(this->object->pushFront(obj));
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::pushFront(V&& obj)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    checkErrorInfo(this->object->moveFront(obj));
    obj.detach();
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::popFront()
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    IBaseObject* obj;
    ErrCode errCode = this->object->popFront(&obj);
    checkErrorInfo(errCode);

    return V(std::move(obj));
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::popBack()
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    IBaseObject* obj;
    ErrCode errCode = this->object->popBack(&obj);
    checkErrorInfo(errCode);

    return V(std::move(obj));
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::insertAt(size_t index, const V& obj)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    ErrCode errCode = this->object->insertAt(index, obj);
    checkErrorInfo(errCode);
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::removeAt(size_t index)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    IBaseObject* obj;
    ErrCode errCode = this->object->removeAt(index, &obj);
    checkErrorInfo(errCode);

    return V(std::move(obj));
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::deleteAt(size_t index)
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    ErrCode errCode = this->object->deleteAt(index);
    checkErrorInfo(errCode);
}

template <class T, class U, class V>
void ListObjectPtr<T, U, V>::clear()
{
    if (!ObjectPtr<T>::object)
        throw InvalidParameterException();

    checkErrorInfo(this->object->clear());
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::operator[](const size_t index)
{
    return getItemAt(index);
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::operator[](const int index)
{
    return getItemAt(index);
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::operator[](const size_t index) const
{
    return getItemAt(index);
}

template <class T, class U, class V>
V ListObjectPtr<T, U, V>::operator[](const int index) const
{
    return getItemAt(index);
}

template <class T, class U, class TValuePtr>
NativeIterator<TValuePtr> ListObjectPtr<T, U, TValuePtr>::begin() const
{
    return this->createStartIteratorInterface();
}

template <class T, class U, class TValuePtr>
NativeIterator<TValuePtr> ListObjectPtr<T, U, TValuePtr>::end() const
{
    return this->createEndIteratorInterface();
}

template <class T, class TValueInterface, class TValuePtr>
IntfID ListObjectPtr<T, TValueInterface, TValuePtr>::getElementInterfaceId() const
{
    if (!this->object)
        throw InvalidParameterException();

    auto elementType = this->template asPtrOrNull<IListElementType>(true);

    IntfID id{};
    if (elementType.assigned())
    {
        ErrCode errCode = elementType->getElementInterfaceId(&id);
        checkErrorInfo(errCode);
    }

    return id;
}

END_NAMESPACE_OPENDAQ
